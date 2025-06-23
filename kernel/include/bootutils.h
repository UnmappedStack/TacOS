#pragma once
#include <kernel.h>
#include <limine.h>

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0};

static volatile struct limine_kernel_address_request kernel_addr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0};

static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST, .revision = 0};

static struct limine_internal_module initrd = {
    .path = "initrd", .flags = LIMINE_INTERNAL_MODULE_REQUIRED};
struct limine_internal_module *module_list = &initrd;
static volatile struct limine_module_request initrd_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
    .internal_modules = &module_list,
    .internal_module_count = 1};

static void init_kernel_info() {
    kernel.memmap_entry_count = memmap_request.response->entry_count;
    kernel.memmap = *(memmap_request.response->entries);
    kernel.hhdm = hhdm_request.response->offset;
    kernel.kernel_phys_addr = kernel_addr_request.response->physical_base;
    kernel.kernel_virt_addr = kernel_addr_request.response->virtual_base;
    kernel.initrd_addr = initrd_request.response->modules[0]->address;
    kernel.rsdp_table = rsdp_request.response->address;
    kernel.scheduler.initiated = false;
}
