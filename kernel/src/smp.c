#include <smp.h>
#include <kernel.h>
#include <cpu.h>
#include <printf.h>
#include <limine.h>

// bsp = base system processor
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
        printf("== PROCESSOR FOUND ==\n");
        printf("Processor ID | %i\n", cpu_info->processor_id);
        printf("LAPIC ID     | %i\n", cpu_info->lapic_id);
        printf("=====================\n");
    }
}
