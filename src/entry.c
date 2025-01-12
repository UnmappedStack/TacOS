#include <mem/pmm.h>
#include <pic.h>
#include <pit.h>
#include <exec.h>
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
#include <fs/ustar.h>
#include <scheduler.h>
#include <fork.h>

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
        if (!buf) break;
    }
}

void cat(char *path) {
    printf("CAT: %s:\n", path);
    VfsFile *f;
    if (!(f = open(path, 0))) {
        printf("Failed to open \"%s\" (in cat).\n");
        HALT_DEVICE();
    }
    size_t off = 0;
    char buf[FILE_DATA_BLOCK_LEN];
    for (;;) {
        int status = vfs_read(f, buf, FILE_DATA_BLOCK_LEN, off);
        if (status == -2) break; // EOF
        printf(buf);
        off += FILE_DATA_BLOCK_LEN;
    }
    close(f);
}

void try_exec_init() {
    printf("Executing init.\n");
    pid_t new_task = fork();
    if (!new_task) {
        printf("Context switch shouldn't yet be enabled, yet the kernel task is already running in a forked task. Halting device.\n");
        HALT_DEVICE();
    }
    Task *task = task_from_pid(new_task);
    if (!task) {
        printf("task_from_pid() failed, couldn't run init program (return NULL)\n");
        HALT_DEVICE();
    }
    if (execve(task, "/usr/bin/init") < 0) {
        printf("Failed to run init program, halting device (expected init program at /usr/bin/init).\n");
        HALT_DEVICE();
    }
}

void _start() {
    DISABLE_INTERRUPTS();
    init_kernel_info();
    init_serial();
    init_pmm();
    init_TSS();
    init_GDT();
    init_IDT();
    init_exceptions();
    init_paging();
    switch_page_structures();
    init_vfs();
    init_pic();
    unpack_initrd();
    init_memregion();
    init_scheduler();
    init_pit();
    ls("/");
    ls("/home");
    try_exec_init();
    unlock_pit();
    ENABLE_INTERRUPTS();
    for (;;);
}
