#include <cpu/idt.h>
#include <mem/pmm.h>
#include <serial.h>
#include <kernel.h>

void set_IDT_entry(uint32_t vector, void *isr, uint8_t flags, IDTEntry *IDT) {
    IDT[vector].offset1 = (uint64_t)isr;
    IDT[vector].offset2 = ((uint64_t)isr) >> 16;
    IDT[vector].offset3 = ((uint64_t)isr) >> 32;
    IDT[vector].flags   = flags;
    IDT[vector].segment_selector = 0x08;
}

extern void syscall_isr();

void init_IDT() {
    kernel.IDT = (IDTEntry*) (kmalloc(1) + kernel.hhdm);
    set_IDT_entry(0x80, &syscall_isr, 0xEF, kernel.IDT);
    IDTR idtr = (IDTR) {
        .size   = (sizeof(IDTEntry) * 256) - 1,
        .offset = (uint64_t) kernel.IDT,
    };
    asm("lidt %0" : : "m" (idtr));
}
