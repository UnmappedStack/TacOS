#include <smp.h>
#include <mem/paging.h>
#include <cpu/gdt.h>
#include <spinlock.h>
#include <kernel.h>
#include <cpu.h>
#include <printf.h>
#include <limine.h>

CPUCore cores[MAX_CORES] __attribute__((section(".data")));

volatile uint64_t num_initialised = 1; // starts at one aka bsp
void ap_entry(struct limine_mp_info *ap_info) {
    static volatile Spinlock ap_init_lock = {0};
    spinlock_acquire(&ap_init_lock);
    DISABLE_INTERRUPTS();
    switch_page_structures();
    init_GDT((uintptr_t)cores[ap_info->lapic_id].stack + PAGE_SIZE * KERNEL_STACK_PAGES);
    load_IDT();
    init_local_apic(kernel.lapic_addr);
    init_lapic_timer();
    lock_lapic_timer();
    printf("Successfully initialised CPU%i!\n", ap_info->lapic_id);
    __atomic_fetch_add(&num_initialised, 1, __ATOMIC_SEQ_CST);
    CURRENT_TASK = cores[0].current_task;
    spinlock_release(&ap_init_lock);
    while (!kernel.init_complete) __builtin_ia32_pause();
    unlock_lapic_timer();
    ENABLE_INTERRUPTS();
    for (;;);
}

void init_smp(void) {
    bool x2apic           = kernel.smp_response->flags & 1;
    uint32_t bsp_lapic_id = kernel.smp_response->bsp_lapic_id;
    uint64_t num_cores    = kernel.smp_response->cpu_count;
    printf("Initiation SMP. Multiprocessor information:\n");
    printf(" - x2apic          | %s\n", (x2apic) ? "enabled" : "disabled");
    printf(" - BSP LAPIC ID    | %i\n", bsp_lapic_id);
    printf(" - Number of cores | %i\n", num_cores);
    for (size_t cpu = 0; cpu < num_cores; cpu++) {
        struct limine_mp_info *cpu_info = kernel.smp_response->cpus[cpu];
        if (cpu_info->lapic_id == 0) continue;
        uint64_t expected = num_initialised;
        cpu_info->goto_address = ap_entry;
        while (num_initialised <= expected)
            __builtin_ia32_pause();
    }
    kernel.num_cores = num_cores;
    printf("All APs initialised\n");
}
