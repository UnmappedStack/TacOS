#pragma once
#include <stddef.h>
#include <stdint.h>
#include <isa/cpu.h>

#define KERNEL_STACK_PAGES 10LL
#define KERNEL_STACK_TOP 0xFFFFFFFFFFFFF000LL
#define KERNEL_STACK_BOTTOM (KERNEL_STACK_TOP - (KERNEL_STACK_PAGES * PAGE_BYTES))

uintptr_t create_address_space(void);
void map_page(uint64_t *pml4vaddr, uintptr_t vaddr, uintptr_t paddr, uint64_t flags);
void map_consecutive_pages(uint64_t *pml4, uintptr_t vmem_start, uintptr_t paddr_start,
                           size_t num_pages, uint64_t flags);
void alloc_consecutive_phys_pages(uint64_t *pml4, uintptr_t vmem_start, size_t num_pages, uint64_t flags);
