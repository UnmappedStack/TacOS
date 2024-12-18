#include <stddef.h>
#include <kernel.h>
#include <serial.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <printf.h>
#include <bootutils.h>

Kernel kernel = {0};

// TODO: Move into a different file like memory.h or smth
const char *mem_types[] = {
    "                Usable",
    "              Reserved",
    "      ACPI Reclaimable",
    "              ACPI NVS",
    "            Bad Memory",
    "Bootloader Reclaimable",
    "Executable and Modules",
    "           Framebuffer",
};

void display_boot_info() {
    printf("HHDM: 0x%p\n", kernel.hhdm);
    printf("There are %i memory map entries.\n", kernel.memmap_entry_count);
    printf("|==================|==================|======================|\n"
           "|      Address     |        Size      |          Type        |\n"
           "|==================|==================|======================|\n");
    for (size_t i = 0; i < kernel.memmap_entry_count; i++)
        printf("|0x%p|0x%p|%s|\n", kernel.memmap[i].base, kernel.memmap[i].length, mem_types[kernel.memmap[i].type]);
    printf("|==================|==================|======================|\n");
}

void _start() {
    init_kernel_info();
    init_serial();
    init_GDT();
    init_IDT();
    display_boot_info();
    for (;;);
}
