#include <serial.h>
#include <gdt.h>
#include <idt.h>
#include <kprintf.h>

void _start(void) {
    serial_init();
    kprintf("this is a kprintf. char=%c, num=%u, str=%s, hex=%x\n", 'A', 69, "hi", 0x69);
    gdt_init();
    idt_init();
    asm volatile("sti\n"
                 "int $0x80\n");
    for (;;);
}
