#include <isa/x86_64/gdt.h>
#include <kprintf.h>

// if one thing mattered least to learn about osdev, it'd be the gdt. the gdt is basically just defining "segments"
// which give privilege levels for different types of code and data, plus the tss which technically does matter but its insignificant.
// It's overall a very legacy x86 thing which doesn't really do much anymore but the cpu still expects it
// to be there, otherwise a general protection fault will occur.

GDTDescriptor gdt_descriptor(uint32_t limit, uint32_t base, uint8_t access, uint8_t flags) {
    GDTDescriptor ret = {0};
    ret.limit1 = limit & 0xffff;
    ret.limit_and_flags = ((limit >> 16) & 0xf) | ((flags & 0xf)<<4);
    ret.base1 = base & 0xffff;
    ret.base2 = (base >> 16) & 0xff;
    ret.base3 = (base >> 24) & 0xff;
    ret.access = access;
    return ret;
}

//TODO: tss
extern void reload_gdt(void);
GDTDescriptor gdt[5] = {0};
__attribute__((noinline)) void gdt_init(void) {
    GDTR gdtr;
    gdt[0] = gdt_descriptor(0,0,0,0);
    gdt[1] = gdt_descriptor(0, 0, 0x9a, 2); // kernel code
    gdt[2] = gdt_descriptor(0, 0, 0x92, 0); // kernel data
    gdt[3] = gdt_descriptor(0, 0, 0xfa, 2); // user code
    gdt[4] = gdt_descriptor(0, 0, 0xf2, 0); // user data

    gdtr.size = sizeof(GDTDescriptor)*5-1;
    gdtr.offset = (uint64_t)gdt;

    __asm__ volatile("lgdt (%0)" : : "r" (&gdtr));
    reload_gdt();
}
