#include <mem/pmm.h>
#include <cpu.h>
#include <stddef.h>
#include <kernel.h>
#include <serial.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <printf.h>
#include <bootutils.h>
#include <mem/paging.h>
#include <panic.h>
#include <fs/vfs.h>
#include <fs/tempfs.h>

Kernel kernel = {0};

void ls(char *path) {
    printf("LS: %s:\n", path);
    VfsDirIter dir;
    VfsFile *buf;
    char fname[MAX_FILENAME_LEN];
    bool is_dir;
    if (opendir(&dir, &buf, path, 0) < 0) {
        printf("Failed to open dir %s\n", path);
        HALT_DEVICE();
    }
    for (;;) {
        vfs_identify(buf, fname, &is_dir);
        char *label = (is_dir) ? " - Directory: " : " - File: ";
        printf("%s%s\n", label, fname);
        buf = vfs_diriter(&dir, &is_dir);
        if (!buf) return;
    }
}

__attribute__((noinline))
void tempfs_test() {
    VfsDrive testdrive;
    testdrive.in_memory = true;
    testdrive.fs = tempfs;
    testdrive.private = tempfs_new();
    printf("Mounting drive /...\n");
    if (vfs_mount("/", testdrive)) {
        printf("Failed to mountd drive.\n");
        HALT_DEVICE();
    }
    printf("Creating some files & subdirectories...\n");
    if (mkdir("/testdir") < 0) {
        printf("Failed to create directory\n");
        HALT_DEVICE();
    }
    mkfile("/testdir/test.txt");
    mkfile("/testdir/test2.txt");
    ls("/testdir");
}

void _start() {
    init_kernel_info();
    init_serial();
    init_GDT();
    init_IDT();
    init_exceptions();
    init_pmm();
    init_paging();
    init_vfs();
    switch_page_structures();
    tempfs_test();
    HALT_DEVICE();
}
