#pragma once
#include <stdint.h>
#include <cpu/idt.h>
#include <cpu/gdt.h>

typedef struct {
    uint64_t GDT[5];
    GDTR     gdtr;
    IDTEntry IDT[256];
} Kernel;

extern Kernel kernel;
