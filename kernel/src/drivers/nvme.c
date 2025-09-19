

// WARNING NOTE INFO IMPORTANT
// DO NOT USE THIS FILE AS A LEARNING RESOURCE
// IT IS INCOMPLETE, BROKEN, AND CURRENTLY, GIVEN UP ON
//
// I REPEAT, DO NOT USE IT AS SOMETHING TO LEARN FROM

#include <printf.h>
#include <mem/pmm.h>
#include <kernel.h>
#include <stdint.h>
#include <pci.h>
#include <stddef.h>
#include <cpu/idt.h>

__attribute__((interrupt))
void nvme_interrupt_handler(void*) {
    printf("Got NVMe interrupt\n");
}

typedef struct {
    uintptr_t addr;
    uint64_t sz;
    uint64_t at; /* the head if it's the completion queue,    */
                 /* or the tail if it's the submission queue. */
} NVMeQueue;

typedef struct {
    uint32_t command;
    uint32_t namespace_id;
    uint64_t rsvd;
    uint64_t metadata_ptr;
    uint64_t prp1;
    uint64_t prp2;
    uint32_t command_specific[6];
} __attribute__((packed)) NVMeSubmissionEntry;

typedef struct {
    uint32_t command_specific;
    uint32_t rsvd;
    uint16_t submission_queue_head_ptr;
    uint16_t submission_queue_identifier;
    uint16_t command_id;
    uint16_t phase_and_status;
} __attribute__((packed)) NVMeCompletionEntry;

typedef struct {
    uintptr_t base;
    NVMeQueue asq;
    NVMeQueue acq;
    uint64_t stride;
    bool phase;
} NVMeDevice;

NVMeDevice nvme_dev = {0};

uint32_t nvme_read_reg(uint32_t offset) {
	volatile uint32_t *ptr = (volatile uint32_t *)(nvme_dev.base + offset);
	return *ptr;
}

void nvme_write_reg(uint32_t offset, uint32_t value) {
	volatile uint32_t *ptr = (volatile uint32_t *)(nvme_dev.base + offset);
	*ptr = value;
}

uint64_t nvme_read_reg64(uint64_t offset) {
	volatile uint64_t *ptr = (volatile uint64_t *)(nvme_dev.base + offset);
	return *ptr;
}

void nvme_write_reg64(uint32_t offset, uint64_t value) {
	volatile uint64_t *ptr = (volatile uint64_t *)(nvme_dev.base + offset);
	*ptr = value;
}

static void nvme_list_capabilities(uint64_t capabilities) {
    printf("Getting controller capabilities:\n");
    uint16_t mqes  = (uint16_t) capabilities;
    bool     cqr   = (capabilities >> 16) & 0b1;
    uint8_t  ams   = (capabilities >> 17) & 0b11;
    uint8_t  to    = (capabilities >> 24) & 0xff;
    uint8_t  dstrt = (capabilities >> 32) & 0xf;
    printf("Max Queue Entries Supported   (MQUES): %i\n", mqes);
    printf("Contiguous Queues Required      (CQR): %s\n", (cqr) ? "True" : "False");
    printf("Arbitration Mechanism Supported (AMS):\n");
    if (ams & 0b01)
        printf("    - Weighted Round Robin with Urgent Priority Class (WRRUPC)\n");
    if (ams & 0b10)
        printf("    - Vendor Specific (VS)\n");
    if (!ams)
        printf("    - Only round robin arbitration is supported.\n");
    printf("Timeout                          (TO): %ims\n", to * 500);
    printf("Doorbell Stride               (DSTRT): 0x%x\n", dstrt);
    nvme_dev.stride = 1 << dstrt;
}

static void create_submission_queue(NVMeQueue *queue) {
    queue->addr = kmalloc(1) + kernel.hhdm;
    memset((void*) queue->addr, 0, PAGE_SIZE);
    queue->sz   = 64;
    queue->at   = 0;
    nvme_write_reg64(0x28, queue->addr - kernel.hhdm);
}

static void create_completion_queue(NVMeQueue *queue) {
    queue->addr = kmalloc(1) + kernel.hhdm;
    memset((void*) queue->addr, 0, PAGE_SIZE);
    queue->sz   = 64;
    queue->at   = 0;
    nvme_write_reg64(0x30, queue->addr - kernel.hhdm);
}

static uint16_t command_id = 69;

// This assumes that there are only two sets of queues, 0=admin & 1=IO.
int nvme_send_command(uint32_t queueID, uint8_t opcode,
        uint64_t prps[2], uint32_t namespaceID, uint32_t args[6]) {
    NVMeQueue *submission_queue, *completion_queue;
    if (!queueID) {
        submission_queue = &nvme_dev.asq;
        completion_queue = &nvme_dev.acq;
    } else {
        printf("TODO: IO queue support\n");
        return -1;
    }
    uint64_t sq_entry_addr = submission_queue->addr + (submission_queue->at * sizeof(NVMeSubmissionEntry));
    NVMeSubmissionEntry entry = {0};
    entry.command = opcode | (command_id++ << 16);
    entry.namespace_id = namespaceID;
    entry.prp1 = prps[0];
    entry.prp2 = prps[1];
    memcpy(entry.command_specific, args, 6 * sizeof(uint32_t));

    memcpy((void*) sq_entry_addr, &entry, sizeof(NVMeSubmissionEntry));
    __sync_synchronize();
    if (++submission_queue->at == submission_queue->sz) submission_queue->at = 0;

    printf("Before doorbell write:\n");
    printf("  Doorbell offset: 0x%x\n", 0x1000 + (2 * queueID) * nvme_dev.stride);
    printf("  Writing tail value: %i\n", submission_queue->at);
    printf("  Command: 0x%x\n", entry.command);
    printf("  PRP1: 0x%x\n", entry.prp1);
    nvme_write_reg(0x1000 + (2 * queueID) * nvme_dev.stride, submission_queue->at);
    // wait for command to complete
    volatile NVMeCompletionEntry *cq = (volatile NVMeCompletionEntry*) completion_queue->addr;
    volatile NVMeCompletionEntry *cqe = &cq[completion_queue->at];
    printf("phase bit is %b\n", nvme_dev.phase);
    while ((cqe->phase_and_status & 0b01) == nvme_dev.phase);
    nvme_dev.phase = !nvme_dev.phase;
    completion_queue->at++;

    nvme_write_reg(0x1000 + 3 * (4 << nvme_dev.stride), completion_queue->at);
    if (completion_queue->at == completion_queue->sz)
        completion_queue->at = 0;
    return cqe->phase_and_status & ~0b1;
}

// it occurs to me it could be problematic that this is using one single IDT vector,
// so if there are multiple NVMe devices, then it would be overwritten each time. (TODO)
void nvme_init(uint8_t bus, uint8_t device, uint8_t function, uint8_t capabilities_list) {
    printf("Found NVMe device, initiating...\n");
    set_IDT_entry(50, (void *)nvme_interrupt_handler, 0x8E, kernel.IDT);
    nvme_dev.base = pci_enable_msi(bus, device, function, capabilities_list, 50);
    nvme_dev.phase = 1;
    if (!nvme_dev.base) {
        printf("NVMe could not be initiated.\n");
        return;
    }

    uint32_t version = nvme_read_reg(0x8);
    uint8_t version_patch  = (uint8_t) version;
    uint8_t version_minor  = (uint8_t) (version >> 8);
    uint16_t version_major = (uint16_t) (version >> 16);
    printf("NVMe version: %i.%i.%i\n", version_major, version_minor, version_patch);
    uint64_t capabilities = (uint64_t) nvme_read_reg(0x0) | ((uint64_t) nvme_read_reg(0x4) << 32);
    nvme_list_capabilities(capabilities); // this also sets the doorbell stride

    create_submission_queue(&nvme_dev.asq);
    create_completion_queue(&nvme_dev.acq);
    uint32_t aqa = ((nvme_dev.asq.sz-1) & 0xfff) | (((nvme_dev.acq.sz-1) & 0xfff) << 16);
    nvme_write_reg(0x24, aqa);
    nvme_write_reg(0x14, nvme_read_reg(0x14) | 0b1); // set enable bit of CC register
    printf("Waiting for controller ready...\n");
    while (!(nvme_read_reg(0x1C) & 0x01));
    printf("Controller ready!\n");

    // send a test command
    int s = nvme_send_command(0, 0x06, (uint64_t[2]) {kmalloc(1), 0}, 0, (uint32_t [6]) {});
    printf("test command returned %x\n", s);
    printf("NVMe device initiated.\n");
}


//int nvme_send_command(uint32_t queueID, uint8_t opcode,
//        uint64_t prps[2], uint32_t namespaceID, uint32_t args[6]) {
