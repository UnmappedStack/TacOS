#pragma once
#include <mem/slab.h>
#include <stddef.h>
#include <stdint.h>
#include <limine.h>
#include <cpu/idt.h>
#include <cpu/gdt.h>
#include <fs/tempfs.h>
#include <scheduler.h>

typedef struct limine_memmap_entry* Memmap;

typedef struct {
    uint64_t       GDT[5];
    GDTR           gdtr; // TODO: Put this on the stack (why didn't I??? lmfao)
    IDTEntry       IDT[256];
    uintptr_t      kernel_phys_addr;
    uintptr_t      kernel_virt_addr;
    uintptr_t      hhdm;
    Memmap         memmap;
    size_t         memmap_entry_count;
    struct list   *pmm_chunklist;
    uintptr_t      cr3;
    struct list   *slab_caches;
    Cache         *tempfs_inode_cache;
    Cache         *tempfs_direntry_cache;
    Cache         *tempfs_data_cache;
    Cache         *vfs_mount_table_cache;
    struct list   vfs_mount_table_list;
    Cache         *vfs_file_cache;
    char          *initrd_addr;
    SchedulerQueue scheduler;
    Cache         *memregion_cache;
} Kernel;

extern Kernel kernel;

void _start();
