#include <pma.h>
#include <string.h>
#include <isa/cpu.h>
#include <mm.h>
#include <paging.h>
#include <smp.h>
#include <kernel.h>
#include <util.h>
#include <lock.h>
#include <limine.h>
#include <kprintf.h>
#include <slab.h>

// TODO: make this more generic to not be limine-specific
static volatile struct limine_mp_request smp_request = {
    .id = LIMINE_MP_REQUEST, .revision = 1};

volatile struct limine_riscv_bsp_hartid_request bsp_id_request = {
    .id = LIMINE_RISCV_BSP_HARTID_REQUEST, .revision = 1};

/* create a CPU* struct for a local processor and store it in gsbase */
CPU *cpu_info_init(uint64_t id) {
    CPU *cpu_info = &kernel_info.processors[id];
    set_current_cpu_info(cpu_info);
    return cpu_info;
}

static uint64_t num_aps_initialised = 0;
static Spinlock init_lock = {0};
/* after the stack has been changed */
void ap_stage2(void) {
#if defined(__x86_64__)
    // this should be moved elsewhere to avoid arch specific code here, TODO
    init_local_interrupt_controller(kernel_info.lapic_addr);
#endif
    timer_local_init();

    num_aps_initialised++;
    while (!kernel_info.schedulers.ready) PAUSE();

    // maybe it'd be better to just make processor_scheduler_init()
    // thread-safe... the granularity could be wayyy better (TODO, but its a
    // microoptimisation anyways tbh since this isnt a hotpath)
    spinlock_acquire(&init_lock);
    processor_scheduler_init();
    spinlock_release(&init_lock);

    ENABLE_INTERRUPTS();
    for (;;);
}

// entry point for all application processors
void ap_entry(struct limine_mp_info *this_cpu) {
    DISABLE_INTERRUPTS();
    isa_early_init();
    SWITCH_PAGE_TREE(kernel_info.vmspace->cr3);
    CPU *cpu = cpu_info_init(get_limine_cpu_id(this_cpu));
    cpu->id  = get_limine_cpu_id(this_cpu);
    ap_stage2();
}

/* starts application processors */
void smp_init(void) {
    klogf(LOG_STATUS, "Initialising APs...\n");
    size_t num_cores = smp_request.response->cpu_count;
    size_t num_pages = PAGE_ALIGN_UP(num_cores * sizeof(CPU)) / PAGE_BYTES;
    kernel_info.processors = (CPU*)vmm_valloc_backed(kernel_info.vmspace, num_pages,
                                                            PAGE_PRESENT | PAGE_WRITE);
    if (!kernel_info.processors) kpanic("failed to allocate backed vmem for kernel_info.processors");
    cpu_info_init(kernel_info.bp_id)->id = kernel_info.bp_id; // it needs to also set up the cpu local struct for the bp here
    for (size_t i = 0; i < num_cores; i++) {
        struct limine_mp_info *cpu = smp_request.response->cpus[i];
        if (get_limine_cpu_id(cpu) == kernel_info.bp_id) continue;
        cpu->goto_address = ap_entry;
    }
    while (num_aps_initialised < num_cores - 1) PAUSE();

    kernel_info.smp_enabled = true;
    klogf(LOG_STATUS, "All application processors initialised.\n");
}

CPU *current_processor(void) {
    return get_current_cpu_info();
}
