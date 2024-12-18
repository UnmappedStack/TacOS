#pragma once
#include <stddef.h>
#include <stdint.h>
#include <limine.h>
#include <cpu/idt.h>
#include <cpu/gdt.h>

typedef struct limine_memmap_entry* Memmap;

typedef struct {
    uint64_t  GDT[5];
    GDTR      gdtr; // TODO: Put this on the stack (why didn't I??? lmfao)
    IDTEntry  IDT[256];
    uintptr_t hhdm;
    Memmap    memmap;
    size_t    memmap_entry_count;
} Kernel;

extern Kernel kernel;
