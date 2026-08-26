#pragma once
#include <isa/cpu.h>

#define PAGE_LEVELS 4ULL

#define SWITCH_PAGE_TREE(cr3) switch_page_tree(cr3);
#define SWITCH_STACK(stack_top) __asm__ volatile("li x2, %0" :: "i"(stack_top));

#define PAGE_WRITE   0b000100
#define PAGE_USER    0b100000
#define PAGE_PRESENT 0b001011 /* both readable and valid (also executable but
                               * this should really be a separate flag later) */
#define INNER_NODE_FLAGS 1 // only valid, none of the other flags

#define PFN_MASK (0xfffffffffff)
#define PAGE_TABLE_ENTRY(paddr, flags) (flags | ((paddr/PAGE_BYTES)<<10))
#define PADDR_FROM_TABLE_ENTRY(entry) (((entry>>10)&PFN_MASK)*PAGE_BYTES)

#define VOFF 0
