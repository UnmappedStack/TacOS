#include <acpi.h>
#include <cpu.h>
#include <kernel.h>
#include <mem/paging.h>
#include <printf.h>

void init_acpi(void) {
    printf("Initiating ACPI... ");
    map_pages(
        (uint64_t *)(kernel.cr3 + kernel.hhdm), (uint64_t)kernel.rsdp_table,
        ((uint64_t)kernel.rsdp_table) - kernel.hhdm, 1, KERNEL_PFLAG_PRESENT);
    if (kernel.rsdp_table->revision) {
        printf("This device uses XSDP, but only RSDP is supported. Halting.\n");
        HALT_DEVICE();
    }
    map_pages((uint64_t *)(kernel.cr3 + kernel.hhdm),
              (uint64_t)kernel.rsdp_table->rsdt_address + kernel.hhdm,
              (uint64_t)kernel.rsdp_table->rsdt_address, 1,
              KERNEL_PFLAG_PRESENT);
    RSDT *rsdt = (RSDT *)(kernel.rsdp_table->rsdt_address + kernel.hhdm);
    kernel.rsdt = rsdt;
    printf("Complete.\n");
}

void *find_MADT(RSDT *root_rsdt) {
    uint64_t num_entries =
        (root_rsdt->header.length - sizeof(root_rsdt->header)) / 4;
    for (size_t i = 0; i < num_entries; i++) {
        ISDTHeader *this_header =
            (ISDTHeader *)(root_rsdt->entries[i] + kernel.hhdm);
        if (!memcmp(this_header->signature, "APIC", 4))
            return (void *)this_header;
    }
    return NULL;
}
