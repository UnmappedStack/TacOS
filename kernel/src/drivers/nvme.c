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
    printf("NVMe device initiated.\n");
}

