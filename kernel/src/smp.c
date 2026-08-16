/* TODO: allocate an indexable array of CPU structs for easy accessability
 * from the other processors, necessary for stuff like load balancing. Might need
 * a non-slab heap for this, pooling or something. */

#include <pma.h>
#include <isa/cpu.h>
#include <mm.h>
#include <paging.h>
#include <smp.h>
#include <panic.h>
#include <kernel.h>
#include <util.h>
#include <lock.h>
#include <limine.h>
#include <kprintf.h>
#include <slab.h>

// TODO: make this more generic to not be limine-specific
static volatile struct limine_mp_request smp_request = {
    .id = LIMINE_MP_REQUEST, .revision = 1};

/* create a CPU* struct for a local processor and store it in gsbase
 * !! Must be under a lock by caller !! */
CPU *cpu_info_init(uint64_t lapic_id) {
    CPU *cpu_info = &kernel_info.processors[lapic_id];
    set_current_cpu_info(cpu_info);
    return cpu_info;
}

static int num_aps_initialised = 0;
static Spinlock init_lock;
/* after the stack has been changed */
void ap_stage2(void) {
    CPU *cpu = current_processor();
#if defined(__x86_64__)
    map_page((uint64_t *)(cpu->cr3 + kernel_info.hhdm),
              (uint64_t)kernel_info.lapic_addr,
              (uint64_t)kernel_info.lapic_addr - kernel_info.hhdm,
              PAGE_PRESENT | PAGE_WRITE);
#endif
    init_local_interrupt_controller(kernel_info.lapic_addr);
    timer_local_init();
    lock_timer();
    DISABLE_INTERRUPTS();
    kprintf("AP%u init OK\n", cpu->id);
    spinlock_release(&init_lock);
    num_aps_initialised++;
    while (!kernel_info.schedulers.ready) IO_WAIT();
    processor_scheduler_init();
    unlock_timer();
    ENABLE_INTERRUPTS();
    for (;;);
}


// entry point for all application processors
void ap_entry(struct limine_mp_info *this_cpu) {
    DISABLE_INTERRUPTS();
    spinlock_acquire(&init_lock);
    isa_early_init();
    CPU *cpu = cpu_info_init(get_limine_cpu_id(this_cpu));
    cpu->id = get_limine_cpu_id(this_cpu);
    cpu->cr3 = create_address_space();
    SWITCH_PAGE_TREE(cpu->cr3);
    SWITCH_STACK(KERNEL_STACK_TOP);
    ap_stage2();
}

/* starts application processors */
void smp_init(void) {
    kprintf("Initialising APs...\n");
    int num_cores = smp_request.response->cpu_count;
    /* TODO: this gives one page max, only allowing for PAGE_BYTES/sizeof(CPU) processors max.
     * Make it expandable once there's a VMA. */
    if ((size_t)num_cores >= PAGE_BYTES/sizeof(CPU)) kpanic("too many processors (fixme)");
    kernel_info.processors = (CPU*)pma_valloc();
    cpu_info_init(0)->id = 0; // it needs to also set up the cpu local struct for the bp here
    for (int i = 0; i < num_cores; i++) {
        struct limine_mp_info *cpu = smp_request.response->cpus[i];
        if (get_limine_cpu_id(cpu) == 0) continue;
        cpu->goto_address = ap_entry;
    }
    while (num_aps_initialised < num_cores - 1) IO_WAIT();

    kernel_info.smp_enabled = true;
    kprintf("All application processors initialised.\n");
}

CPU *current_processor(void) {
    return get_current_cpu_info();
}
