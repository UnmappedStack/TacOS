#pragma once

#include <apic.h>
#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KERNEL_PFLAG_PRESENT 0b00000001
#define KERNEL_PFLAG_WRITE 0b00000010
#define KERNEL_PFLAG_USER 0b00000100
#define KERNEL_PFLAG_WRITE_COMBINE 0b10000000

#define KERNEL_STACK_ADDR ((uintptr_t)cores[get_current_processor()].stack)
#define KERNEL_STACK_PTR  (KERNEL_STACK_ADDR + KERNEL_STACK_PAGES * PAGE_SIZE)

#define USER_STACK_PAGES 10LL
#define USER_STACK_ADDR (USER_STACK_PTR - USER_STACK_PAGES * PAGE_SIZE)
#define USER_STACK_PTR 0x700000000000LL

#define PAGE_ALIGN_DOWN(addr)                                                  \
    ((addr / 4096) * 4096) // works cos of integer division
#define PAGE_ALIGN_UP(x) ((((x) + 4095) / 4096) * 4096)
#define bytes_to_pages(x) (((x) + (PAGE_SIZE - 1)) / PAGE_SIZE)

#define KERNEL_SWITCH_PAGE_TREE(TREE_ADDRESS)                                  \
    __asm__ volatile("movq %0, %%cr3" : : "r"(TREE_ADDRESS))

#define KERNEL_SWITCH_STACK(STACK_PTR)                                         \
    __asm__ volatile("movq %0, %%rsp\n"                                        \
                     "movq $0, %%rbp\n"                                        \
                     :                                                         \
                     : "r"(STACK_PTR))

void map_pages(uint64_t pml4_addr[], uint64_t virt_addr, uint64_t phys_addr,
               uint64_t num_pages, uint64_t flags);
void alloc_pages(uint64_t pml4_addr[], uint64_t virt_addr, uint64_t num_pages,
                 uint64_t flags);
void init_paging(void);
uint64_t *init_paging_task(void);
uint64_t virt_to_phys(uint64_t pml4_addr[], uint64_t virt_addr);
void write_vmem(uint64_t *pml4_addr, uint64_t virt_addr, char *data,
                size_t len);
void read_vmem(uint64_t *pml4_addr, uintptr_t virt_addr, char *buffer,
               size_t len);
void push_vmem(uint64_t *pml4_addr, uint64_t rsp, char *data, size_t len);
void clear_page_cache(uint64_t addr);
uintptr_t valloc(size_t size_pages);

#define switch_page_structures()                                               \
    KERNEL_SWITCH_PAGE_TREE(kernel.cr3);
