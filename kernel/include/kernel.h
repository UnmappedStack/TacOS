#pragma once
#include <smp.h>
#include <slab.h>
#include <scheduler.h>
#include <list.h>
#include <framebuffer.h>
#include <isa/cpu.h>

// shared kernel struct for everything global.
// this will probably also contain locking information later when there's smp etc
typedef struct {
    Framebuffer framebuffers[MAX_FRAMEBUFFERS];
    int num_framebuffers;

    /* pmm stuff */
    struct list pmm_nodes;
    uintptr_t hhdm;
    struct limine_memmap_response *memmap;

    /* vmem stuff */
    uint64_t cr3;

    /* ACPI & APIC stuff */
#if defined(__x86_64__)
    RSDP *rsdp_table;
    RSDT *rsdt;
    IOApic ioapic_device;
    uintptr_t ioapic_addr;
    uintptr_t lapic_addr;
    uint64_t pit_counter; // for lapic timer calibration
#endif
    uintptr_t cpu_id;

#if defined(__x86_64__)
    IDTGate idt[256];
#endif
    GlobalSchedulerInfo schedulers;
    CPU *processors;

    bool smp_enabled; // if not then we shouldn't send out IPIs on panic
} KernelInfo;

extern KernelInfo kernel_info;
