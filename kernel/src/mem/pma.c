// physical memory allocator for TacOS.
// this uses a linked list allocator for fast O(1) allocations with no memory overhead of single pages.
// continuous pages are not needed because it can be threaded together into continuous
// pages within virtual memory.

#include <panic.h>
#include <kernel.h>
#include <util.h>
#include <pma.h>
#include <kprintf.h>
#include <limine.h>

// the limine bootloader will detect this and fill it with the necessary info about 
// the memory map and the kernel_info.hhdm
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 4};
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0};

const char *types_stringified[] = {
    [LIMINE_MEMMAP_USABLE]                 = "Usable",
    [LIMINE_MEMMAP_RESERVED]               = "Reserved",
    [LIMINE_MEMMAP_ACPI_RECLAIMABLE]       = "ACPI reclaimable",
    [LIMINE_MEMMAP_ACPI_NVS]               = "ACPI NVS",
    [LIMINE_MEMMAP_BAD_MEMORY]             = "Bad memory",
    [LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE] = "Bootloader reclaimable",
    [LIMINE_MEMMAP_KERNEL_AND_MODULES]     = "Executable and modules",
    [LIMINE_MEMMAP_FRAMEBUFFER]            = "Framebuffer",
};

void pma_init(void) {
    kernel_info.memmap = memmap_request.response;
    struct limine_memmap_entry **entries = memmap_request.response->entries;
    size_t num_entries = memmap_request.response->entry_count;

    kernel_info.hhdm = hhdm_request.response->offset;

    kprintf("Memory map dump:\n");
    list_init(&kernel_info.pmm_nodes);
    for (size_t i = 0; i < num_entries; i++) {
        uint64_t base   = entries[i]->base;
        uint64_t length = entries[i]->length;
        uint64_t type   = entries[i]->type;
        size_t size_pages = length / PAGE_BYTES;
        kprintf(" -> %x (%u pages): %s\n", base, size_pages, types_stringified[type]);
        if (type != LIMINE_MEMMAP_USABLE) continue;
        PMMNode *node = (PMMNode*) (base + kernel_info.hhdm);
        node->size_pages = size_pages;
        list_insert(&kernel_info.pmm_nodes, &node->list);
    }
    kprintf("PMA init OK\n");
}

// allocate one physical page
uintptr_t pma_palloc(void) {
    if (list_empty(&kernel_info.pmm_nodes))
        kpanic("Out of Memory");

    PMMNode *node = CONTAINER_OF(kernel_info.pmm_nodes.prev, PMMNode, list);
    list_remove(&node->list);

    if (node->size_pages > PAGE_BYTES) {
        PMMNode *new_node = (PMMNode*) ((uintptr_t)node + PAGE_BYTES);
        new_node->size_pages = node->size_pages - 1;
        if (new_node->size_pages)
            list_insert(&kernel_info.pmm_nodes, &new_node->list);
    }
    
    return (uintptr_t)node - kernel_info.hhdm;
}

// free one physical page.
// this assumes that it is a valid memory block so
// it is the caller's responsibility if something is wrong.
void pma_pfree(uintptr_t ptr) {
    PMMNode *node = (PMMNode*) (ptr + kernel_info.hhdm);
    node->size_pages = 1;
    list_insert(&kernel_info.pmm_nodes, &node->list);
}

// allocate one physical page, returning a virtual address
uintptr_t pma_valloc(void) {
    return pma_palloc() + kernel_info.hhdm;
}

void pma_vfree(uintptr_t ptr) {
    pma_pfree(ptr - kernel_info.hhdm);
}
