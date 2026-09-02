#include <kernel.h>
#include <limine.h>
#include <isa/x86_64/acpi.h>
#include <string.h>
#include <paging.h>
#include <kprintf.h>
#include <util.h>

static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST, .revision = 0};

void acpi_init(void) {
    kernel_info.rsdp_table = (RSDP*) rsdp_request.response->address;
    map_page(
        (uint64_t *)(kernel_info.cr3 + kernel_info.hhdm), (uint64_t)kernel_info.rsdp_table,
        ((uint64_t)kernel_info.rsdp_table) - kernel_info.hhdm, PAGE_PRESENT);
    klogf(LOG_DEBUG, "Detected ACPI revision %u\n", kernel_info.rsdp_table->revision);
    uintptr_t phys_sdt_addr = (kernel_info.rsdp_table->revision) ?
                                    kernel_info.rsdp_table->xsdt_address :
                                    kernel_info.rsdp_table->rsdt_address;
    map_page((uint64_t *)(kernel_info.cr3 + kernel_info.hhdm),
              phys_sdt_addr + kernel_info.hhdm,
              phys_sdt_addr,
              PAGE_PRESENT);
    XSDT *xsdt = (XSDT *)(phys_sdt_addr + kernel_info.hhdm);
    kernel_info.xsdt = xsdt;
    klogf(LOG_STATUS, "ACPI init OK\n");
}

void *find_MADT(XSDT *root_xsdt) {
    bool xsdt = (kernel_info.rsdp_table->revision) ? true : false; // false if rsdt instead
    uint64_t num_entries = (root_xsdt->header.length - sizeof(root_xsdt->header)) / ((xsdt)?8:4);
    for (size_t i = 0; i < num_entries; i++) {
        uintptr_t entry;
        if (xsdt) entry = ((uint64_t*)root_xsdt->entries)[i];
        else      entry = ((uint32_t*)root_xsdt->entries)[i];
        ISDTHeader *this_header =
            (ISDTHeader *)(entry + kernel_info.hhdm);
        if (!memcmp(this_header->signature, "APIC", 4))
            return (void *)this_header;
    }
    return NULL;
}
