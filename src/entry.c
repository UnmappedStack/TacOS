#include <kernel.h>
#include <serial.h>
#include <cpu/gdt.h>
#include <printf.h>

Kernel kernel = {0};

void _start() {
    init_serial();
    init_GDT();
    for (;;);
}
