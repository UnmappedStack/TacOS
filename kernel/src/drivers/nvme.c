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
} NVMeQueue;

typedef struct {
    uint32_t command;
    uint32_t namespace_id;
    uint64_t rsvd;
    uint64_t metadata_ptr;
    uint64_t data_ptr0; // ->these are PRPs
    uint64_t data_ptr1; // ---^
    uint32_t command_specific[6];
} __attribute__((packed)) NVMeSubmissionEntry;

typedef struct {
    uint32_t command_specific;
    uint32_t rsvd;
    uint16_t submission_queue_head_ptr;
    uint16_t submission_queue_identifier;
    uint16_t command_id;
    uint16_t status_and_phase_bit;
} NVMeCompletionEntry;

typedef struct {
    uintptr_t base;
    NVMeQueue submission_queue;
    NVMeQueue completion_queue;
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
}

static void create_submission_queue(NVMeQueue *queue) {
    queue->addr = kmalloc(1) + kernel.hhdm;
    queue->sz   = 63;
    nvme_write_reg64(0x28, queue->addr);
}

static void create_completion_queue(NVMeQueue *queue) {
    queue->addr = kmalloc(1) + kernel.hhdm;
    queue->sz   = 63;
    nvme_write_reg64(0x30, queue->addr);
}

// it occurs to me it could be problematic that this is using one single IDT vector,
// so if there are multiple NVMe devices, then it would be overwritten each time. (TODO)
void nvme_init(uint8_t bus, uint8_t device, uint8_t function, uint8_t capabilities_list) {
    printf("Found NVMe device, initiating...\n");
    set_IDT_entry(50, (void *)nvme_interrupt_handler, 0x8E, kernel.IDT);
    nvme_dev.base = pci_enable_msi(bus, device, function, capabilities_list, 50);
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
    nvme_list_capabilities(capabilities);

    create_submission_queue(&nvme_dev.submission_queue);
    create_completion_queue(&nvme_dev.completion_queue);
    printf("NVMe device initiated.\n");
}

