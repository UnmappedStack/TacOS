#include <cpu/gdt.h>
#include <tss.h>
#include <kernel.h>
#include <mem/pmm.h>
#include <printf.h>
#include <stdint.h>

extern void reload_gdt();

static uint64_t create_gdt_entry(uint64_t base, uint64_t limit, uint64_t access,
                                 uint64_t flags) {
    uint64_t base1 = base & 0xFFFF;
    uint64_t base2 = (base >> 16) & 0xFF;
    uint64_t base3 = (base >> 24) & 0xFF;
    uint64_t limit1 = limit & 0xFFFF;
    uint64_t limit2 = (limit >> 16) & 0b1111;
    uint64_t entry = 0;
    entry |= limit1;
    entry |= limit2 << 48;
    entry |= base1 << 16;
    entry |= base2 << 32;
    entry |= base3 << 56;
    entry |= access << 40;
    entry |= flags << 52;
    return entry;
}

void create_system_segment_descriptor(uint64_t *GDT, uint8_t idx, uint64_t base,
                                      uint64_t limit, uint64_t access,
                                      uint64_t flags) {
    uint64_t limit1 = limit & 0xFFFF;
    uint64_t limit2 = (limit >> 16) & 0b1111;
    uint64_t base1 = base & 0xFFFF;
    uint64_t base2 = (base >> 16) & 0xFF;
    uint64_t base3 = (base >> 24) & 0xFF;
    uint64_t base4 = (base >> 32) & 0xFFFFFFFF;
    GDT[idx] = 0;
    GDT[idx] |= limit1;
    GDT[idx] |= base1 << 16;
    GDT[idx] |= base2 << 32;
    GDT[idx] |= access << 40;
    GDT[idx] |= (limit2 & 0xF) << 48;
    GDT[idx] |= (flags & 0xF) << 52;
    GDT[idx] |= base3 << 56;
    GDT[idx + 1] = base4;
}

__attribute__((noinline)) void load_GDT(GDTR *gdtr) {
    // inline assembly because when has that ever been a dumb idea :P
    __asm__ volatile("lgdt (%0)" : : "r"(gdtr));
    reload_gdt();
    __asm__ volatile("mov $0x28, %%ax\nltr %%ax" : : : "eax");
}

void init_GDT(uintptr_t kernel_rsp) {
    uint64_t *GDT = (uint64_t *)(kmalloc(1) + kernel.hhdm);
    GDT[0] = create_gdt_entry(0, 0, 0, 0);      // null
    GDT[1] = create_gdt_entry(0, 0, 0x9A, 0x2); // kernel code
    GDT[2] = create_gdt_entry(0, 0, 0x92, 0);   // kernel data
    GDT[3] = create_gdt_entry(0, 0, 0xFA, 0x2); // user code
    GDT[4] = create_gdt_entry(0, 0, 0xF2, 0);   // user data
    create_system_segment_descriptor(GDT, 5, (uint64_t)init_TSS(kernel_rsp),
                                     sizeof(TSS) - 1, 0x89, 0);
    GDTR gdtr = (GDTR){.size = (sizeof(uint64_t) * 7) - 1,
                         .offset = (uint64_t)GDT};
    load_GDT(&gdtr);
}
