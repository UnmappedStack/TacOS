#include <smp.h>
#include <panic.h>
#include <kernel.h>
#include <msr.h>
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
void cpu_info_init(void) {
    CPU *cpu_info = slab_alloc(kernel_info.cpu_cache);
    wrmsr(GSBASE, (uint64_t)cpu_info);
}

// entry point for all application processors
static int num_aps_initialised = 0;
static Spinlock init_lock;
void ap_entry(struct limine_mp_info *this_cpu) {
    spinlock_acquire(&init_lock);
    cpu_info_init();
    kprintf("AP%u init OK\n", this_cpu->lapic_id);
    spinlock_release(&init_lock);
    num_aps_initialised++;
    for (;;);
}

/* starts application processors */
void smp_init(void) {
    kernel_info.cpu_cache = cache_create(sizeof(CPU));
    cpu_info_init(); // it needs to also set up the cpu local struct for the bp here
    int num_cores = smp_request.response->cpu_count;
    for (int i = 0; i < num_cores; i++) {
        struct limine_mp_info *cpu = smp_request.response->cpus[i];
        if (cpu->lapic_id == 0) continue;
        cpu->goto_address = ap_entry;
    }
    while (num_aps_initialised < num_cores - 1) IO_WAIT();
    kprintf("All application processors initialised.\n");
}

CPU *current_processor(void) {
    CPU *ret = (CPU*) rdmsr(GSBASE);
    if (ret == NULL) {
        kpanic("GSBase not set yet");
    }
    return ret;
}
