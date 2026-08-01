#include <serial.h>
#include <slab.h>
#include <paging.h>
#include <panic.h>
#include <framebuffer.h>
#include <util.h>
#include <gdt.h>
#include <idt.h>
#include <pma.h>
#include <kprintf.h>
#include <kernel.h>

KernelInfo kernel_info = {0};

void _start(void) {
    serial_init();
    framebuffer_init();
    gdt_init();
    idt_init();
    exceptions_init();
    pma_init();
    pma_palloc();

    kernel_info.cr3 = create_address_space();
    SWITCH_PAGE_TREE(kernel_info.cr3);
    kprintf("Page tree switched successfully\n");

    init_acpi();
    init_apic();

    FREEZE_DEVICE();
}
