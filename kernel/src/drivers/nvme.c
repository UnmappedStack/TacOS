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

// it occurs to me it could be problematic that this is using one single IDT vector,
// so if there are multiple NVMe devices, then it would be overwritten each time. (TODO)
void nvme_init(uint8_t bus, uint8_t device, uint8_t function, uint8_t capabilities_list) {
    printf("Found NVMe device, initiating...\n");
    set_IDT_entry(50, (void *)nvme_interrupt_handler, 0x8E, kernel.IDT);
    if (pci_enable_msi(bus, device, function, capabilities_list, 50) < 0) {
        printf("NVMe could not be initiated.\n");
        return;
    }
    printf("NVMe device initiated.\n");
}

