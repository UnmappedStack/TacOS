#pragma once
#include <stddef.h>
#include <stdint.h>

/* page flags */
#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE   (1 << 1)
#define PAGE_USER    (1 << 2)

#define SWITCH_PAGE_TREE(TREE_ADDRESS) \
    __asm__ volatile("movq %0, %%cr3" : : "r"(TREE_ADDRESS))

uintptr_t create_address_space(void);
void map_page(uint64_t *pml4vaddr, uintptr_t vaddr, uintptr_t paddr, uint64_t flags);
void map_consecutive_pages(uint64_t *pml4, uintptr_t vmem_start, uintptr_t paddr_start,
                           size_t num_pages, uint64_t flags);
