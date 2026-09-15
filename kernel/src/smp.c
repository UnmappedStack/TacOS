#include <pma.h>
#include <string.h>
#include <assert.h>
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

void handle_single_ipi(IPIMessage *message) {
    CPU *cpu = get_current_cpu_info();
    switch (message->type) {
    case IPI_HALT:
        klogf(LOG_ERROR, "Halt CPU%u\n", cpu->id);
        FREEZE_DEVICE();
        return;
    default:
        klogf(LOG_WARN, "unhandled ipi");
        return;
    }
}

void ipi_handler(void) {
    CPU *cpu = get_current_cpu_info();
    IPIQueue *queue = &cpu->ipi_queue;

    /* this should probably ideally be lockless later since its
     * inside an interrupt (TODO) */
    MCSSpinlock ipi_local_lock;
    mcs_spinlock_acquire(&queue->lock, &ipi_local_lock);

    for (size_t i = 0; i < MAX_IPI_MESSAGES; i++) {
        IPIMessage *message = &queue->messages[i];

        if (message->type == IPI_NONE) continue;
       
        handle_single_ipi(message);
        message->type = IPI_NONE; // mark it as read
    }

    // reset writing back to the start
    queue->upto = 0;

    mcs_spinlock_release(&queue->lock, &ipi_local_lock);
}

/* TODO: send to specific processors instead of all of them */
void ipi_send(IPIMessage message) {
    MCSSpinlock local_lock = {0};

    for (size_t cpu = 0; cpu < kernel_info.num_cores; cpu++) {
        IPIQueue *queue = &kernel_info.processors[cpu].ipi_queue;
        /* to be honest this could probably be done locklessly but I don't think
         * it matters that much hopefully. might be worth considering in the
         * future. */
        mcs_spinlock_acquire(&queue->lock, &local_lock);

        memcpy(&queue->messages[queue->upto], &message, sizeof(IPIMessage));
        if (queue->upto >= MAX_IPI_MESSAGES) queue->upto = 0;

        mcs_spinlock_release(&queue->lock, &local_lock);
    }

    // actually send it (this needs to later be able to go to a specific cpu,
    // not just all of them)
    ipi_all();
}

void halt_all_processors(void) {
    ipi_send((IPIMessage) {
        .type = IPI_HALT,
    });
}

/* create a CPU* struct for a local processor and store it in gsbase */
CPU *cpu_info_init(uint64_t id) {
    CPU *cpu_info = &kernel_info.processors[id];
    memset(&cpu_info->ipi_queue, 0, sizeof(IPIQueue));
    set_current_cpu_info(cpu_info);
    return cpu_info;
}

static uint64_t num_aps_initialised = 0;
static MCSSpinlock init_lock = {0};
/* after the stack has been changed */
void ap_stage2(void) {
    init_local_interrupt_controller(kernel_info.lapic_addr);
    timer_local_init();

    num_aps_initialised++;
    while (!kernel_info.schedulers.ready) PAUSE();

    // maybe it'd be better to just make processor_scheduler_init()
    // thread-safe... the granularity could be wayyy better (TODO, but its a
    // microoptimisation anyways tbh since this isnt a hotpath)
    MCSSpinlock local_init_lock;
    mcs_spinlock_acquire(&init_lock, &local_init_lock);
    processor_scheduler_init();
    mcs_spinlock_release(&init_lock, &local_init_lock);

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
    kernel_info.num_cores = smp_request.response->cpu_count;
    size_t num_pages = PAGE_ALIGN_UP(kernel_info.num_cores * sizeof(CPU)) / PAGE_BYTES;
    kernel_info.processors = (CPU*)vmm_valloc_backed(kernel_info.vmspace, num_pages,
                                                            PAGE_PRESENT | PAGE_WRITE);
    if (!kernel_info.processors) kpanic("failed to allocate backed vmem for kernel_info.processors");
    cpu_info_init(kernel_info.bp_id)->id = kernel_info.bp_id; // it needs to also set up the cpu local struct for the bp here
    for (size_t i = 0; i < kernel_info.num_cores; i++) {
        struct limine_mp_info *cpu = smp_request.response->cpus[i];
        if (get_limine_cpu_id(cpu) == kernel_info.bp_id) continue;
        cpu->goto_address = ap_entry;
    }
    while (num_aps_initialised < kernel_info.num_cores - 1) PAUSE();

    kernel_info.smp_enabled = true;
    klogf(LOG_STATUS, "All application processors initialised.\n");
}

CPU *current_processor(void) {
    return get_current_cpu_info();
}
