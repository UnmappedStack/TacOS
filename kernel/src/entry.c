#include <apic.h>
#include <cpu/msr.h>
#include <syscalls.h>
#include <bootutils.h>
#include <cpu.h>
#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <exec.h>
#include <fork.h>
#include <framebuffer.h>
#include <fs/device.h>
#include <fs/tempfs.h>
#include <fs/ustar.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <keyboard.h>
#include <mem/paging.h>
#include <mem/pmm.h>
#include <panic.h>
#include <pit.h>
#include <printf.h>
#include <scheduler.h>
#include <serial.h>
#include <stddef.h>
#include <string.h>
#include <tty.h>

extern void enable_sse(void);

Kernel kernel = {0};

void ls(char *path) {
    printf("LS: %s:\n", path);
    VfsDirIter dir;
    VfsFile *buf;
    size_t fsize;
    char fname[MAX_FILENAME_LEN];
    bool is_dir;
    if (opendir(&dir, &buf, path, 0) < 0) {
        printf("Failed to open dir %s\n", path);
        HALT_DEVICE();
    }
    for (;;) {
        vfs_identify(buf, fname, &is_dir, &fsize);
        char *label = (is_dir) ? " - Directory" : " - File";
        printf("%s (%i bytes): %s\n", label, fsize, fname);
        buf = vfs_diriter(&dir, &is_dir);
        printf("did diriter\n");
        if (!buf)
            break;
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
        if (status == -2)
            break; // EOF
        printf(buf);
        off += FILE_DATA_BLOCK_LEN;
    }
    close(f);
}

void try_exec_init(void) {
    printf("Executing init.\n");
    pid_t new_task = fork(NULL);
    if (!new_task) {
        printf("Context switch shouldn't yet be enabled, yet the kernel task "
               "is already running in a forked task. Halting device.\n");
        HALT_DEVICE();
    }
    Task *task = task_from_pid(new_task);
    if (!task) {
        printf("task_from_pid() failed, couldn't run init program (return "
               "NULL)\n");
        HALT_DEVICE();
    }
    char *env[] = {"TACOS=yum", "TORTILLAS=ripoff", NULL};
    if (execve(task, "/usr/bin/init",
               (char *[]){"/usr/bin/init", "YOU_ARE_INIT", NULL}, env) < 0) {
        printf("Failed to run init program, halting device (expected init "
               "program at /usr/bin/init).\n");
        HALT_DEVICE();
    }
    printf("task ptr = 0x%p", task);
}

void _start(void) {
    DISABLE_INTERRUPTS();
    init_kernel_info();
    init_serial();
    init_msr();
    init_pmm();
    init_TSS();
    init_GDT();
    init_IDT();
    enable_sse();
    init_exceptions();
    init_paging();
    switch_page_structures();
    init_acpi();
    init_apic();
    init_vfs();
    unpack_initrd();
    init_devices();
    init_memregion();
    init_scheduler();
    init_apic();
    init_pit();
    init_syscalls();
    ls("/");
    ls("/home");
    init_framebuffer();
    init_tty();
    init_keyboard();
    try_exec_init();
    init_lapic_timer();
    printf("Successful boot, init spawned, enabling scheduler to enter "
           "userspace\n");
    ENABLE_INTERRUPTS();
    unlock_lapic_timer();
    for (;;);
}
