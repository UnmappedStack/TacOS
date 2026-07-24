#include <idt.h>
#include <kprintf.h>

// interrupts are basically signals that the cpu sends the kernel every time an event happens
// so that you can stop what you are doing and handle it. The IDT is a table of all the interrupts
// and pointers to the handlers.

// offset  -> a pointer to the handler
// segment -> matches to the segment which should be used to respond to it in the gdt
// flags   -> gate type, ring level, and present bit
IDTGate idt_descriptor(uint64_t offset, uint16_t segment, uint8_t flags) {
    IDTGate ret = {0};
    ret.offset1 = offset & 0xffff;
    ret.offset2 = (offset >> 16) & 0xffff;
    ret.offset3 = offset >> 32;
    ret.segment = segment;
    ret.flags   = flags;
    return ret;
}

__attribute__((interrupt))
void test_isr(void*) {
    kprintf("got interrupt call!\n");
}

// once there's smp, these will need to be stored per-cpu
static IDTGate idt[256] = {0};
static IDTR idtr;
void idt_init(void) {
    // idt[isr] will set interrupt vector <isr> to the handler described by idt_descriptor().
    // this is dynamic so we can add to this even after loading the idt.
    idt[0x80] = idt_descriptor((uint64_t)test_isr, 0x08, 0xef);

    // the idtr is just a small table with the size and location of the idt
    // which is loaded into a register for it using the lidt instruction.
    idtr.offset = (uint64_t)idt;
    idtr.size = sizeof(IDTGate)*256-1;
    __asm__ volatile("lidt %0" : : "m"(idtr));

    kprintf("IDT init OK\n");
}
