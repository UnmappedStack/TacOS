#pragma once
#include <kernel.h>
#include <limine.h>

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static volatile struct limine_kernel_address_request kernel_addr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

static void init_kernel_info() {
    kernel.memmap_entry_count = memmap_request.response->entry_count;
    kernel.memmap             = *(memmap_request.response->entries);
    kernel.hhdm               = hhdm_request.response->offset;
    kernel.kernel_phys_addr   = kernel_addr_request.response->physical_base;
    kernel.kernel_virt_addr   = kernel_addr_request.response->virtual_base;
}
