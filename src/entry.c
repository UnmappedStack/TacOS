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

__attribute__((noinline))
void tempfs_test() {
    VfsDrive testdrive;
    testdrive.in_memory = true;
    testdrive.fs = tempfs;
    testdrive.private = tempfs_new();
    printf("Testing mounting drive /dev\n");
    if (vfs_mount("/", testdrive)) {
        printf("Failed to mountd drive.\n");
        HALT_DEVICE();
    }
    if (mkdir("/dev") < 0) {
        printf("Failed to create directory\n");
        HALT_DEVICE();
    } else {
        printf("Success.\n");
    }
    if (mkdir("/dev/path") < 0) {
        printf("Failed to create directory\n");
        HALT_DEVICE();
    } else {
        printf("Success.\n");
    }
    printf("Trying to open with O_CREAT...\n");
    printf("File = %p\n", open("/dev/path/testfile.txt", O_CREAT));
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
