#pragma once
#include <smp.h>
#include <slab.h>
#include <scheduler.h>
#include <apic.h>
#include <idt.h>
#include <acpi.h>
#include <list.h>
#include <framebuffer.h>

// shared kernel struct for everything global.
// this will probably also contain locking information later when there's smp etc
typedef struct {
    Framebuffer framebuffers[MAX_FRAMEBUFFERS];

    /* pmm stuff */
    struct list pmm_nodes;
    uintptr_t hhdm;
    struct limine_memmap_response *memmap;

    /* vmem stuff */
    uint64_t cr3;

    /* ACPI & APIC stuff */
    RSDP *rsdp_table;
    RSDT *rsdt;
    uintptr_t lapic_addr;
    IOApic ioapic_device;
    uintptr_t ioapic_addr;
    uint64_t pit_counter; // for lapic timer calibration

    IDTGate idt[256];
    GlobalSchedulerInfo schedulers;
    CPU *processors;
} KernelInfo;

extern KernelInfo kernel_info;
