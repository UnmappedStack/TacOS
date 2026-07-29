#pragma once
#include <stdint.h>

#define SWITCH_PAGE_TREE(TREE_ADDRESS)                                  \
    __asm__ volatile("movq %0, %%cr3" : : "r"(TREE_ADDRESS))

uintptr_t create_address_space(void);
