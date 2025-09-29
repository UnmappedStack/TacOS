#pragma once
#include <acpi.h>
#include <apic.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <framebuffer.h>
#include <fs/tempfs.h>
#include <kernel.h>
#include <limine.h>
#include <mem/slab.h>
#include <pit.h>
#include <scheduler.h>
#include <stddef.h>
#include <stdint.h>
#include <tss.h>
#include <tty.h>

typedef struct limine_memmap_entry *Memmap;

typedef struct {
    uint64_t *GDT;
    IDTEntry *IDT;
    TSS *tss;
    uintptr_t kernel_phys_addr;
    uintptr_t kernel_virt_addr;
    uintptr_t hhdm;
    Memmap memmap;
    size_t memmap_entry_count;
    struct list *pmm_chunklist;
    uintptr_t cr3;
    struct list *slab_caches;
    Cache *tempfs_inode_cache;
    Cache *tempfs_direntry_cache;
    Cache *tempfs_data_cache;
    Cache *vfs_mount_table_cache;
    struct list vfs_mount_table_list;
    Cache *vfs_file_cache;
    char *initrd_addr;
    SchedulerQueue scheduler;
    Cache *memregion_cache;
    Framebuffer framebuffer;
    TTY tty;
    RSDP *rsdp_table;
    RSDT *rsdt;
    struct timespec global_clock;
    uintptr_t lapic_addr;
    IOApic ioapic_device;
    uintptr_t ioapic_addr;
    size_t pit_counter;
    size_t vmm_upto;
    Cache *ipc_socket_cache;
    Cache *ipc_socketqueueitem_cache;
    Cache *usrptys_cache;
    struct limine_mp_response *smp_response;
} Kernel;

extern Kernel kernel;

void _start(void);
