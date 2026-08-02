#pragma once
#include <stddef.h>
#include <stdint.h>

#define KERNEL_STACK_PAGES 10LL
#define KERNEL_STACK_TOP 0xFFFFFFFFFFFFF000LL
#define KERNEL_STACK_BOTTOM (KERNEL_STACK_TOP - (KERNEL_STACK_PAGES * PAGE_BYTES))

/* page flags */
#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE   (1 << 1)
#define PAGE_USER    (1 << 2)

#define SWITCH_PAGE_TREE(TREE_ADDRESS) \
    __asm__ volatile("movq %0, %%cr3" : : "r"(TREE_ADDRESS))

#define SWITCH_STACK(STACK_TOP) \
    __asm__ volatile("movq %0, %%rsp;" \
                     "movq $0, %%rbp" : : "r"(STACK_TOP));

uintptr_t create_address_space(void);
void map_page(uint64_t *pml4vaddr, uintptr_t vaddr, uintptr_t paddr, uint64_t flags);
void map_consecutive_pages(uint64_t *pml4, uintptr_t vmem_start, uintptr_t paddr_start,
                           size_t num_pages, uint64_t flags);
void alloc_consecutive_phys_pages(uint64_t *pml4, uintptr_t vmem_start, size_t num_pages, uint64_t flags);
