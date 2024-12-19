#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limine.h>
 
#define KERNEL_PFLAG_PRESENT 0b1
#define KERNEL_PFLAG_WRITE   0b10
#define KERNEL_PFLAG_USER    0b100

#define PAGE_SIZE 4096
#define KERNEL_STACK_PAGES 2LL
#define KERNEL_STACK_PTR 0xFFFFFFFFFFFFF000LL
#define KERNEL_STACK_ADDR KERNEL_STACK_PTR-(KERNEL_STACK_PAGES*PAGE_SIZE)

#define USER_STACK_PAGES 2LL
#define USER_STACK_ADDR (USER_STACK_PTR - USER_STACK_PAGES*PAGE_SIZE)
#define USER_STACK_PTR 0x700000000000LL

#define PAGE_ALIGN_DOWN(addr) ((addr / 4096) * 4096) // works cos of integer division
#define PAGE_ALIGN_UP(x) ((((x) + 4095) / 4096) * 4096)

#define KERNEL_SWITCH_PAGE_TREE(TREE_ADDRESS) \
    __asm__ volatile (\
       "movq %0, %%cr3"\
       :\
       :  "r" (TREE_ADDRESS)\
    )

#define KERNEL_SWITCH_STACK(STACK_PTR) \
    __asm__ volatile (\
       "movq %0, %%rsp\n"\
       "movq $0, %%rbp\n"\
       "push $0"\
       :\
       :  "r" (STACK_PTR)\
    )

void map_pages(uint64_t pml4_addr[], uint64_t virt_addr, uint64_t phys_addr, uint64_t num_pages, uint64_t flags);

void alloc_pages(uint64_t pml4_addr[], uint64_t virt_addr, uint64_t num_pages, uint64_t flags);

void init_paging();

uint64_t* init_paging_task();

uint64_t virt_to_phys(uint64_t pml4_addr[], uint64_t virt_addr);

void write_vmem(uint64_t *pml4_addr, uint64_t virt_addr, char *data, size_t len);

void push_vmem(uint64_t *pml4_addr, uint64_t rsp, char *data, size_t len);

void clear_page_cache(uint64_t addr);

#define switch_page_structures() \
    printf("Switching CR3 & kernel stack...\n"); \
    KERNEL_SWITCH_PAGE_TREE(kernel.cr3); \
    KERNEL_SWITCH_STACK(KERNEL_STACK_PTR); \
    printf("Successfully switched page structures.\n");