#include <printf.h>
#include <kernel.h>
#include <stdint.h>
#include <pci.h>
#include <stddef.h>
#include <cpu/idt.h>

__attribute__((interrupt))
void nvme_interrupt_handler(void*) {
    printf("Got NVMe interrupt\n");
}

uintptr_t nvme_base = 0;

uint32_t nvme_read_reg(uint32_t offset) {
	volatile uint32_t *ptr = (volatile uint32_t *)(nvme_base + offset);
	return *ptr;
}

void nvme_write_reg(uint32_t offset, uint32_t value) {
	volatile uint32_t *ptr = (volatile uint32_t *)(nvme_base + offset);
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
        printf("    - None\n");
    printf("Timeout                          (TO): %ims\n", to * 500);
    printf("Doorbell Stride               (DSTRT): 0x%x\n", dstrt);
}

// it occurs to me it could be problematic that this is using one single IDT vector,
// so if there are multiple NVMe devices, then it would be overwritten each time. (TODO)
void nvme_init(uint8_t bus, uint8_t device, uint8_t function, uint8_t capabilities_list) {
    printf("Found NVMe device, initiating...\n");
    set_IDT_entry(50, (void *)nvme_interrupt_handler, 0x8E, kernel.IDT);
    nvme_base = pci_enable_msi(bus, device, function, capabilities_list, 50);
    if (!nvme_base) {
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
    printf("NVMe device initiated.\n");
    for (;;);
}

