#pragma once
#include <smp.h>
#include <vmm.h>
#include <vmm.h>
#include <tty.h>
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
    GlobalTTYState tty_state;

    /* pmm stuff */
    LList pmm_nodes;
    uintptr_t hhdm;
    struct limine_memmap_response *memmap;

    /* vmem stuff */
    VMSpace *vmspace; // this will later be per-process
    Cache *rbtree_cache;
    Cache *vmregion_cache;
    Cache *vmspace_cache;

    /* ACPI & APIC stuff */
#if defined(__x86_64__)
    RSDP *rsdp_table;
    XSDT *xsdt;
    IOApic ioapic_device;
    uintptr_t ioapic_addr;
    uintptr_t lapic_addr;
    uint64_t pit_counter; // for lapic timer calibration
#endif

    /* DTB stuff */
#if defined(__riscv)
    uint64_t timebase_freq;
#endif

#if defined(__x86_64__)
    IDTGate idt[256];
#endif
    GlobalSchedulerInfo schedulers;
    CPU *processors;
    uint64_t bp_id;

    bool smp_enabled; // if not then we shouldn't send out IPIs on panic
} KernelInfo;

extern KernelInfo kernel_info;
