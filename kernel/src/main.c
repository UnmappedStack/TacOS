#include <serial.h>
#include <smp.h>
#include <scheduler.h>
#include <scheduler.h>
#include <slab.h>
#include <paging.h>
#include <framebuffer.h>
#include <util.h>
#include <isa/cpu.h>
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
    framebuffer_init();

    isa_early_init();
    exceptions_init();
    pma_init();

    kernel_info.cr3 = create_address_space();
    SWITCH_PAGE_TREE(kernel_info.cr3);
    SWITCH_STACK(KERNEL_STACK_TOP);
    kprintf("Page tree switched successfully\n");
    
    power_management_init();
    interrupt_controller_init();
    timer_init();

    smp_init();
    global_scheduler_init();
    processor_scheduler_init();

    add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 10, 0));
    add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 15, 0));
    add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 20, 0));

    unlock_timer();
    ENABLE_INTERRUPTS();
    for (;;);

    FREEZE_DEVICE();
}
