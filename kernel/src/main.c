#include <serial.h>
#include <rbtree.h>
#include <vmm.h>
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

void __assert_fail(const char *assertion, const char *file, uint32_t line, const char *fn) {
    if (kernel_info.smp_enabled) halt_all_processors();

    klogf(LOG_ERROR, "Assertion failed at %s:%u in %s: %s\n", file, line, fn, assertion);
    FREEZE_DEVICE();
}

void __stack_chk_fail(void) {
    if (kernel_info.smp_enabled) halt_all_processors();

    print_string("Stack smashing detected\n");
    FREEZE_DEVICE();
}

void boot_stage2(void) {
    klogf(LOG_STATUS, "Page tree switched successfully\n");

    power_management_init();
    interrupt_controller_init();
    timer_init();
    smp_init();
    global_scheduler_init();
    processor_scheduler_init();

    add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 10, 0));
    add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 15, 0));
    add_thread_to_current_processor(create_thread(SCHED_TIMESHARE, 20, 0));

    ENABLE_INTERRUPTS();
    for (;;);

    FREEZE_DEVICE();
}

void _start(void) {
    DISABLE_INTERRUPTS();
    serial_init();
    framebuffer_init();

    isa_early_init();
    exceptions_init();
    pma_init();
    rbtree_init();

    kernel_info.vmspace = create_virtual_memory_space();
    SWITCH_PAGE_TREE(kernel_info.vmspace->cr3);

    boot_stage2();
}
