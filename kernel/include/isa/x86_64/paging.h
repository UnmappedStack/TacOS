#pragma once

/* page flags */
#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE   (1 << 1)
#define PAGE_USER    (1 << 2)
#define INNER_NODE_FLAGS (PAGE_PRESENT | PAGE_WRITE | PAGE_USER)

#define SWITCH_PAGE_TREE(TREE_ADDRESS) \
    __asm__ volatile("movq %0, %%cr3" : : "r"(TREE_ADDRESS))

#define SWITCH_STACK(STACK_TOP) \
    __asm__ volatile("movq %0, %%rsp;" \
                     "movq $0, %%rbp" : : "r"(STACK_TOP));

#define PAGE_TABLE_ENTRY(paddr, flags) (flags | paddr)

// TODO: this wont support higher flags like execute disable properly
#define PADDR_FROM_TABLE_ENTRY(entry) (PAGE_ALIGN_DOWN(entry))
