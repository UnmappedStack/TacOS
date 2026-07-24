#include <idt.h>
#include <kprintf.h>

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

static IDTGate idt[256] = {0};
static IDTR idtr;
void idt_init(void) {
    idt[0x80] = idt_descriptor((uint64_t)test_isr, 0x08, 0xef);
    idtr.offset = (uint64_t)idt;
    idtr.size = sizeof(IDTGate)*256-1;
    __asm__ volatile("lidt %0" : : "m"(idtr));
    kprintf("IDT init OK\n");
}
