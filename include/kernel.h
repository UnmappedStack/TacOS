#pragma once
#include <stdint.h>
#include <cpu/gdt.h>

typedef struct {
    uint64_t GDT[5];
    GDTR     gdtr;
} Kernel;

extern Kernel kernel;
