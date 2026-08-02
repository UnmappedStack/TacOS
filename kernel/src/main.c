#include <serial.h>
#include <scheduler.h>
#include <slab.h>
#include <pit.h>
#include <paging.h>
#include <panic.h>
#include <framebuffer.h>
#include <util.h>
#include <gdt.h>
#include <idt.h>
#include <msr.h>
#include <pma.h>
#include <kprintf.h>
#include <kernel.h>

KernelInfo kernel_info = {0};

void __stack_chk_fail(void) {
    print_string("Stack smashing detected\n");
    FREEZE_DEVICE();
}

void _start(void) {
    serial_init();

    if (!cpu_has_msr())
        kpanic("MSRs not supported");
    else kprintf("MSRs supported!\n");

    framebuffer_init();
    gdt_init();
    idt_init();
    exceptions_init();
    pma_init();

    kernel_info.cr3 = create_address_space();
    SWITCH_PAGE_TREE(kernel_info.cr3);
    SWITCH_STACK(KERNEL_STACK_TOP);
    kprintf("Page tree switched successfully\n");
    
    acpi_init();
    apic_init();
    pit_init();
    init_local_apic(kernel_info.lapic_addr);
    init_lapic_timer();

    global_scheduler_init();
    processor_scheduler_init();

    unlock_lapic_timer();
    ENABLE_INTERRUPTS();
    for (;;);

    FREEZE_DEVICE();
}
