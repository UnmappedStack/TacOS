#include <mm.h>
#include <string.h>
#include <pma.h>
#include <stddef.h>
#include <kernel.h>

// x86_64-specific virtual memory mapping and related functionality which
// the vmm can be built upon

/* page flags */
#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE   (1 << 1)
#define PAGE_USER    (1 << 2)

/* macro utility ops */
#define PAGE_ALIGN_DOWN(addr) ((addr / 4096) * 4096)
#define PAGE_ALIGN_UP(x) ((((x) + 4095) / 4096) * 4096)

/* vaddr is the virtual address we're trying to map to,
 * tlevel is the pml table level we're getting the index of,
 * and it should return a specific index of the set of that pml level */
#define TABLE_FROM_VADDR(vaddr, tlevel) ((vaddr >> (12+9*(tlevel-1))) & 511)

uint64_t *get_or_create_next_layer(uint64_t *parent_layer, uint64_t idx) {
    /* if the index of a page tree level needed does not already exist,
     * make it with full permissions, and allocate for the next level's table to
     * be used for further child tables. */
    if (!parent_layer[idx]) {
        uintptr_t child_paddr = pma_palloc();
        parent_layer[idx] = PAGE_PRESENT | PAGE_WRITE | PAGE_USER | child_paddr;
        memset((void*)(child_paddr + kernel_info.hhdm), 0, 512);
    }
    // once we know it exists, we can return it
    return (uint64_t*) PAGE_ALIGN_DOWN(parent_layer[idx] + kernel_info.hhdm);
}

void map_page(uint64_t *pml4vaddr, uintptr_t vaddr, uintptr_t paddr, uint64_t flags) {
    vaddr &= ~0xFFFF000000000000; /* high bits must be cleared as they are used for other stuff */

    uint64_t *current_layer_vaddr = pml4vaddr;
    for (uint8_t pml_level = 4; pml_level > 1; pml_level--) {
        current_layer_vaddr = get_or_create_next_layer(
                                  current_layer_vaddr,
                                  TABLE_FROM_VADDR(vaddr, pml_level)
                              );
    }

    /* now that we've actually got the pml1 table and the offset into it, we can
     * just add the mapping to the page tree */
    current_layer_vaddr[TABLE_FROM_VADDR(vaddr, 1)] = paddr | flags;
}

// This could probably be faster, but I feel like this is the more readable implementation
void map_consecutive_pages(uint64_t *pml4, uintptr_t vmem_start, uintptr_t paddr_start,
                           size_t num_pages, uint64_t flags) {
    for (size_t i = 0; i < num_pages; i++)
        map_page(pml4, vmem_start + i * PAGE_BYTES, paddr_start + i * PAGE_BYTES, flags);
}
