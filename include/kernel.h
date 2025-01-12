#pragma once
#include <tss.h>
#include <framebuffer.h>
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
    uint64_t      *GDT;
    IDTEntry      *IDT;
    TSS           *tss;
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
    Framebuffer    framebuffer;
    uint64_t        char_x;
    uint64_t       char_y;
} Kernel;

extern Kernel kernel;

void _start();
