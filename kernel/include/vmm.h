#pragma once
#include <stdint.h>

// a full virtual memory space, one for each process
typedef struct {
    uintptr_t cr3; // *technically* cr3 only refers to x86 but I use it to just
                   // refer to the physical address of the highest page table level
                   // (usually pml4 or potentially pml5)
} __attribute__((aligned(16))) VMSpace;

VMSpace *create_virtual_memory_space(void);
