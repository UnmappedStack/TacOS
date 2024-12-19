#include <mem/pmm.h>
#include <stddef.h>
#include <kernel.h>
#include <serial.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <printf.h>
#include <bootutils.h>

Kernel kernel = {0};

void _start() {
    init_kernel_info();
    init_serial();
    init_GDT();
    init_IDT();
    init_pmm();
    for (size_t i = 0; i < 20; i++) {
        printf("Allocated address: 0x%p\n", kmalloc(i));
    }
    for (;;);
}
