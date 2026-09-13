#include <mm.h>
#include <kprintf.h>
#include <paging.h>
#include <isa/cpu.h>
#include <limine.h>
#include <string.h>
#include <pma.h>
#include <stddef.h>
#include <kernel.h>

// ISA agnostic virtual memory mapping and related functionality which
// the vmm can be built upon

// the limine bootloader will detect this and fill it with the necessary info about 
// the location of the kernel in physical and virtual memory
static volatile struct limine_kernel_address_request kernel_addr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0};

// references to the linker script labels which mark sections
extern uint64_t readonly_start[];
extern uint64_t readonly_end[];
extern uint64_t writable_start[];
extern uint64_t writable_end[];

#define TABLE_FROM_VADDR(vaddr, tlevel) (((vaddr) >> (VOFF+9*(tlevel-1))) & 511)


uint64_t *get_or_create_next_layer(uint64_t *parent_layer, uint64_t idx) {
    /* if the index of a page tree level needed does not already exist,
     * make it with full permissions, and allocate for the next level's table to
     * be used for further child tables. */
    if (!parent_layer[idx]) {
        uintptr_t child_paddr = pma_palloc();
        memset((void*)(child_paddr + kernel_info.hhdm), 0, PAGE_BYTES);

        parent_layer[idx] = PAGE_TABLE_ENTRY(
                                child_paddr,
                                INNER_NODE_FLAGS
                            );
    }
    // once we know it exists, we can return it
    uint64_t paddr = PADDR_FROM_TABLE_ENTRY((uintptr_t)parent_layer[idx]);
    return (uint64_t*) (paddr + kernel_info.hhdm);
}

void map_page(uint64_t *pml4vaddr, uintptr_t vaddr, uintptr_t paddr, uint64_t flags) {
    vaddr &= ~0xFFFF000000000000ULL; /* high bits must be cleared as they are used for other stuff */

    uint64_t *current_layer_vaddr = pml4vaddr;
#if defined(__riscv)
    vaddr /= PAGE_BYTES; // on riscv it needs the virtual frame number, not the address
#endif
    for (uint8_t pml_level = 4; pml_level > 1; pml_level--) {
        current_layer_vaddr = get_or_create_next_layer(
                                  current_layer_vaddr,
                                  TABLE_FROM_VADDR(vaddr, pml_level)
                              );
    }

    /* now that we've actually got the pml1 table and the offset into it, we can
     * just add the mapping to the page tree */
    current_layer_vaddr[TABLE_FROM_VADDR(vaddr, 1)] = PAGE_TABLE_ENTRY(paddr, flags);
}

// This could probably be faster, but I feel like this is the more readable implementation
void map_consecutive_pages(uint64_t *pml4, uintptr_t vmem_start, uintptr_t paddr_start,
                           size_t num_pages, uint64_t flags) {
    for (size_t i = 0; i < num_pages; i++)
        map_page(pml4, vmem_start + i * PAGE_BYTES, paddr_start + i * PAGE_BYTES, flags);
}

// allocates a bunch of non-consecutive physical pages and strings them together in vmem
void alloc_consecutive_phys_pages(uint64_t *pml4, uintptr_t vmem_start, size_t num_pages, uint64_t flags) {
    for (size_t i = 0; i < num_pages; i++) {
        uintptr_t phys_page = pma_palloc();
        map_page(pml4, vmem_start + i * PAGE_BYTES, phys_page, flags);
    }
}

// Maps one section of the kernel binary into the virtual memory space
void map_kernel_section(uint64_t *pml4, uint64_t start, uint64_t end, uint64_t flags) {
    uintptr_t kernel_paddr = kernel_addr_request.response->physical_base;
    uintptr_t kernel_vaddr = kernel_addr_request.response->virtual_base;

    uint64_t length = PAGE_ALIGN_UP(end) - start;
    uint64_t paddr  = kernel_paddr + (start - kernel_vaddr);

    map_consecutive_pages(pml4, start, paddr, length / PAGE_BYTES, flags);
}

// Maps the kernel binary into a virtual memory space
void map_kernel_into_vspace(uint64_t *pml4) {
    uint64_t kernel_readonly_start = (uint64_t) readonly_start;
    uint64_t kernel_readonly_end   = (uint64_t) readonly_end;
    uint64_t kernel_writable_start = (uint64_t) writable_start;
    uint64_t kernel_writable_end   = (uint64_t) writable_end;

    map_kernel_section(pml4, kernel_readonly_start, kernel_readonly_end, PAGE_PRESENT);
    map_kernel_section(pml4, kernel_writable_start, kernel_writable_end, PAGE_PRESENT | PAGE_WRITE);
}

// maps all memory that could be used into a virtual memory space
void map_all_memory_into_vspace(uint64_t *pml4) {
    struct limine_memmap_entry **entries = kernel_info.memmap->entries;
    size_t num_entries = kernel_info.memmap->entry_count;
    for (size_t i = 0; i < num_entries; i++) {
        uintptr_t paddr = entries[i]->base;
        uintptr_t vaddr = entries[i]->base + kernel_info.hhdm;
        uint64_t  type  = entries[i]->type;
        if (type == LIMINE_MEMMAP_BAD_MEMORY || type == LIMINE_MEMMAP_RESERVED) continue;
        map_consecutive_pages(pml4, vaddr, paddr, entries[i]->length/PAGE_BYTES, PAGE_PRESENT | PAGE_WRITE);
    }
}

// Creates a new address space and maps essential memory into it
uintptr_t create_address_space(void) {
    uintptr_t pml4_paddr = pma_palloc();
    uint64_t *pml4 = (uint64_t*) (pml4_paddr + kernel_info.hhdm);
    memset(pml4, 0, PAGE_BYTES);

    map_all_memory_into_vspace(pml4);
    map_kernel_into_vspace(pml4);
   
    return pml4_paddr;
}
