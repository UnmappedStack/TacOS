#pragma once

#define SWITCH_PAGE_TREE(cr3) FREEZE_DEVICE()
#define SWITCH_STACK(stack_top) FREEZE_DEVICE()

#define PAGE_WRITE   0b000100
#define PAGE_USER    0b100000
#define PAGE_PRESENT 0b001011 /* both readable and valid (also executable but
                               * this should really be a separate flag later) */

#define PFN_MASK (0xfffffffffff)
#define PAGE_TABLE_ENTRY(paddr, flags) (flags | ((paddr/PAGE_BYTES)<<10))
#define PADDR_FROM_TABLE_ENTRY(entry) (((entry>>10)&PFN_MASK)*PAGE_BYTES)
