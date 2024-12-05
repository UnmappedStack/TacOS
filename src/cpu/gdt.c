#include <cpu/gdt.h>
#include <stdint.h>
#include <kernel.h>

static uint64_t create_gdt_entry(uint64_t base, uint64_t limit, uint64_t access, uint64_t flags) {
    uint64_t base1  = base & 0xFFFF;
    uint64_t base2  = (base >> 16) & 0xFF;
    uint64_t base3  = (base >> 24) & 0xFF;
    uint64_t limit1 = limit & 0xFFFF;
    uint64_t limit2 = limit >> 16;
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

__attribute__((noinline))
void init_GDT() {
    kernel.GDT[0] = create_gdt_entry(0, 0, 0, 0); // null
    kernel.GDT[1] = create_gdt_entry(0, 0, 0x9A, 0x2); // kernel code
    kernel.GDT[2] = create_gdt_entry(0, 0, 0x92, 0); // kernel data
    kernel.GDT[3] = create_gdt_entry(0, 0, 0xFA, 0x2); // user code
    kernel.GDT[4] = create_gdt_entry(0, 0, 0xF2, 0); // user data
    kernel.gdtr = (GDTR) {
        .size = (sizeof(uint64_t) * 5) - 1,
        .offset = (uint64_t) &kernel.GDT
    };
    // inline assembly because when has that ever been a dumb idea :P
    asm("lgdt (%0)" : : "r" (&kernel.gdtr));  
    asm volatile("push $0x08; \
              lea .reload_CS(%%rip), %%rax; \
              push %%rax; \
              retfq; \
              .reload_CS: \
              mov $0x10, %%ax; \
              mov %%ax, %%ds; \
              mov %%ax, %%es; \
              mov %%ax, %%fs; \
              mov %%ax, %%gs; \
              mov %%ax, %%ss" : : : "eax", "rax");
}
