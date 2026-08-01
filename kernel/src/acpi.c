#include <kernel.h>
#include <limine.h>
#include <acpi.h>
#include <string.h>
#include <paging.h>
#include <kprintf.h>
#include <util.h>

/* TODO: map_pages stuff here needs to later be done with the generic vmm,
 * not x86_64 specific stuff */

static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST, .revision = 0};

void acpi_init(void) {
    kernel_info.rsdp_table = (RSDP*) rsdp_request.response->address;
    map_page(
        (uint64_t *)(kernel_info.cr3 + kernel_info.hhdm), (uint64_t)kernel_info.rsdp_table,
        ((uint64_t)kernel_info.rsdp_table) - kernel_info.hhdm, PAGE_PRESENT);
    if (kernel_info.rsdp_table->revision) {
        kprintf("This device uses XSDP, but only RSDP is supported. Halting.\n");
        FREEZE_DEVICE();
    }
    map_page((uint64_t *)(kernel_info.cr3 + kernel_info.hhdm),
              (uint64_t)kernel_info.rsdp_table->rsdt_address + kernel_info.hhdm,
              (uint64_t)kernel_info.rsdp_table->rsdt_address,
              PAGE_PRESENT);
    RSDT *rsdt = (RSDT *)(kernel_info.rsdp_table->rsdt_address + kernel_info.hhdm);
    kernel_info.rsdt = rsdt;
    kprintf("ACPI init OK\n");
}

void *find_MADT(RSDT *root_rsdt) {
    uint64_t num_entries =
        (root_rsdt->header.length - sizeof(root_rsdt->header)) / 4;
    for (size_t i = 0; i < num_entries; i++) {
        ISDTHeader *this_header =
            (ISDTHeader *)(root_rsdt->entries[i] + kernel_info.hhdm);
        if (!memcmp(this_header->signature, "APIC", 4))
            return (void *)this_header;
    }
    return NULL;
}
