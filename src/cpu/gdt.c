#include <printf.h>
#include <cpu/gdt.h>
#include <mem/pmm.h>
#include <stdint.h>
#include <kernel.h>

extern void reload_gdt();

static uint64_t create_gdt_entry(uint64_t base, uint64_t limit, uint64_t access, uint64_t flags) {
    uint64_t base1  = base & 0xFFFF;
    uint64_t base2  = (base >> 16) & 0xFF;
    uint64_t base3  = (base >> 24) & 0xFF;
    uint64_t limit1 = limit & 0xFFFF;
    uint64_t limit2 = (limit >> 16) & 0b1111;
    uint64_t entry  = 0;
    entry |= limit1;
    entry |= limit2 << 48;
    entry |= base1  << 16;
    entry |= base2  << 32;
    entry |= base3  << 56;
    entry |= access << 40;
    entry |= flags  << 52;
    return entry;
}

void create_system_segment_descriptor(uint64_t *GDT, uint8_t idx, uint64_t base, uint64_t limit, uint64_t access, uint64_t flags) {
    uint64_t limit1 = limit & 0xFFFF;
    uint64_t limit2 = (limit >> 16) & 0b1111;
    uint64_t base1  = base & 0xFFFF;
    uint64_t base2  = (base >> 16) & 0xFF;
    uint64_t base3  = (base >> 24) & 0xFF;
    uint64_t base4  = (base >> 32) & 0xFFFFFFFF;
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

__attribute__((noinline))
void init_GDT() {
    GDTR gdtr;
    kernel.GDT = (uint64_t*) (kmalloc(1) + kernel.hhdm);
    kernel.GDT[0] = create_gdt_entry(0, 0, 0, 0); // null
    kernel.GDT[1] = create_gdt_entry(0, 0, 0x9A, 0x2); // kernel code
    kernel.GDT[2] = create_gdt_entry(0, 0, 0x92, 0); // kernel data
    kernel.GDT[3] = create_gdt_entry(0, 0, 0xFA, 0x2); // user code
    kernel.GDT[4] = create_gdt_entry(0, 0, 0xF2, 0); // user data
    create_system_segment_descriptor(kernel.GDT, 5, (uint64_t) &kernel.tss, sizeof(TSS) - 1, 0x89, 0);
    gdtr = (GDTR) {
        .size = (sizeof(uint64_t) * 7) - 1,
        .offset = (uint64_t) kernel.GDT
    };
    // inline assembly because when has that ever been a dumb idea :P
    asm volatile("lgdt (%0)" : : "r" (&gdtr));
    asm volatile("mov $0x28, %%ax\nltr %%ax" : : : "eax");
    printf("gdt: %p\n", &kernel.GDT[0]);
    reload_gdt();
}
