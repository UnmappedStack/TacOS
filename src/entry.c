#include <kernel.h>
#include <serial.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <printf.h>

Kernel kernel = {0};

void _start() {
    init_serial();
    init_GDT();
    init_IDT();
    asm("int $0x80");
    for (;;);
}
