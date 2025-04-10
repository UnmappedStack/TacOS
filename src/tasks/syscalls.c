#include <exec.h>
#include <fork.h>
#include <cpu.h>
#include <kernel.h>
#include <scheduler.h>
#include <stdint.h>
#include <printf.h>

#define CURRENT_TASK kernel.scheduler.current_task

int remove_child(Task *parent, pid_t child, bool in_wait, int status) {
    for (size_t i = 0; i < MAX_CHILDREN; i++) {
        if (parent->children[i].pid == child) {
            if (in_wait) // called from wait syscall
                parent->children[i].pid = 0;
            else
                parent->children[i].status = status;
            return 0;
        }
    }
    return -1;
}

int sys_open(char *filename, int flags, int mode) {
    (void) mode;
    uint64_t file_descriptor = 0;
    Task *current_task = CURRENT_TASK;
    for (; file_descriptor < MAX_RESOURCES; file_descriptor++) {
        if (current_task->resources[file_descriptor].f) continue;
        current_task->resources[file_descriptor].f = open((char*) filename, flags);
        current_task->resources[file_descriptor].offset = 0;
        printf("Opened %s to file descriptor %i\n", filename, file_descriptor);
        return file_descriptor;
    }
    printf("Couldn't open file %s\n", filename);
    return -1;
}

int sys_close(int fd) {
    return close(CURRENT_TASK->resources[fd].f);
}

int sys_read(int fd, char *buf, size_t count) {
    return vfs_read(CURRENT_TASK->resources[fd].f, buf, count, 0);
}

size_t sys_write(int fd, char *buf, size_t count) {
    printf("Args: %i, %p, %i\n", (uint64_t) fd, buf, count); 
    return vfs_write(CURRENT_TASK->resources[fd].f, buf, count, 0);
}

void sys_exit(int status) {
    // TODO: Clean up and report to parent
    if (CURRENT_TASK->pid <= 1) {
        DISABLE_INTERRUPTS();
        printf("Init task exited! Halting device. (Exited with status %i, 0x%x)\n", status, status);
        HALT_DEVICE();
    }
    printf("Exited task with status %i\n", status);
    remove_child(CURRENT_TASK->parent, CURRENT_TASK->pid, false, status);
    CURRENT_TASK->flags |= TASK_DEAD;
    CURRENT_TASK->flags &= ~TASK_PRESENT;
    for (;;);
}

int sys_getpid() {
    return CURRENT_TASK->pid;
}

int sys_execve(char *path) {
    return execve(CURRENT_TASK, path);
}

// TODO: Actually send a signal to the task and do clean up, report to it's parent
int sys_kill(int pid, int sig) {
    if (pid < 0) {
        printf("Tried to kill process group, but not supported yet.\n");
        return -1;
    } else if (pid <= 1) {
        printf("Killed init task! Halting device.\n");
        HALT_DEVICE();
        return -1;
    } else {
        Task *to_kill = task_from_pid(pid);
        to_kill->flags &= ~TASK_PRESENT;
        to_kill->flags |= TASK_DEAD;
        remove_child(to_kill->parent, to_kill->pid, false, sig);
        printf("Killed task %i with signal %i\n", pid, sig);
        return 0;
    }
}

int sys_isatty(int fd) {
    VfsFile *f = CURRENT_TASK->resources[fd].f;
    if (!f) return 0;
    if (f->drive.fs.fs_id == fs_tempfs) {
        TempfsInode *private = f->private;
        return private->type == Device && private->devops.is_term;
    }
    return 0;
}

int sys_wait(int *status) {
    for (;;) {
        for (size_t i = 0; i < MAX_CHILDREN; i++) {
            if (CURRENT_TASK->children[i].pid &&
               (task_from_pid(CURRENT_TASK->children[i].pid)->flags & TASK_DEAD)) {
                pid_t pid = CURRENT_TASK->children[i].pid;
                if (status)
                    *status = CURRENT_TASK->children[i].status;
                remove_child(CURRENT_TASK->parent, CURRENT_TASK->children[i].pid, true, 0);
                return (int) pid;
            }
        }
        IO_WAIT();
    }
}

uintptr_t sys_sbrk(intptr_t increment) {
    if (!increment) {
        printf("sbrk return %p (inc 0)\n", CURRENT_TASK->program_break);
        return CURRENT_TASK->program_break;
    }
    uintptr_t previous_break = CURRENT_TASK->program_break;
    CURRENT_TASK->program_break += increment;
    if (!(CURRENT_TASK->program_break % PAGE_SIZE) ||
         (PAGE_ALIGN_DOWN(CURRENT_TASK->program_break) > PAGE_ALIGN_DOWN(previous_break))) {
        size_t num_new_pages = PAGE_ALIGN_UP(increment) / 4096;
        alloc_pages((uint64_t*) (CURRENT_TASK->pml4 + kernel.hhdm), CURRENT_TASK->program_break, num_new_pages, KERNEL_PFLAG_WRITE | KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_USER);
        add_memregion(&CURRENT_TASK->memregion_list, CURRENT_TASK->program_break, num_new_pages, true, KERNEL_PFLAG_WRITE | KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_USER);
    }
    printf("sbrk return %p\n", previous_break);
    return previous_break;
}

/* TODO: Right now, this just deletes a file.
 * See https://man7.org/linux/man-pages/man2/unlink.2.html for how this should actually work.
 * This currently isn't posix compliant. */
int sys_unlink(char *path) {
    VfsFile *f = open(path, 0);
    return rm_file(f);
}

int sys_remove(char *filename) {
    VfsFile *f = vfs_access(filename, 0, 0);
    if (!f) return -1;
    bool is_dir;
    if (vfs_identify(f, NULL, &is_dir, NULL)) return -1;
    if (is_dir) {
        VfsDirIter dir = vfs_file_to_diriter(f);
        return rm_dir(&dir);
    } else {
        return rm_file(f);
    }
}

int sys_mkdir(char *filename, size_t mode) {
    (void) mode;
    return mkdir(filename);
}

typedef enum {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
} FileLocs;
size_t sys_lseek(int fd, size_t offset, int whence) {
    size_t fsize;
    switch (whence) {
        case SEEK_SET:
            CURRENT_TASK->resources[fd].offset = offset;
            return offset;
        case SEEK_CUR:
            CURRENT_TASK->resources[fd].offset += offset;
            return CURRENT_TASK->resources[fd].offset;
        case SEEK_END:
            vfs_identify(CURRENT_TASK->resources[fd].f, NULL, NULL, &fsize);
            CURRENT_TASK->resources[fd].offset = fsize + offset;
            return CURRENT_TASK->resources[fd].offset;
        default:
            printf("Invalid whence value for lseek\n");
            sys_exit(1);
            return 1;
    }
}

void sys_invalid(int sys) {
    printf("Invalid syscall: %i\n", sys);
}

/* called by the task switch. Don't ask why this is in the syscalls.c file lol */
void increment_global_clock(void) {
    // since this is called about every ms, add one ms to it
    const size_t frequency = 1000000; // it's called every `frequency` nanoseconds
    const size_t ns_in_s   = 1000000000;
    kernel.global_clock.tv_nsec += frequency;
    if (kernel.global_clock.tv_nsec >= ns_in_s) {
        kernel.global_clock.tv_sec++;
        kernel.global_clock.tv_nsec -= ns_in_s;
    }
}

/* TODO: Stop ignoring the clockid and actually have separate clocks */
int sys_clock_gettime(size_t clockid, struct timespec *tp) {
    (void) clockid;
    tp->tv_sec  = kernel.global_clock.tv_sec;
    tp->tv_nsec = kernel.global_clock.tv_nsec;
    return 0;
}

// TODO: actually yield properly
void sys_sched_yield(void) {
    for (size_t i = 0; i < 3; i++)
        asm volatile("hlt\n");
}

// major stub
void* sys_mmap(void *addr, size_t length, int prot, int flags,
                  int fd, size_t offset) {
    (void) prot; // TODO: don't ignore prot and map (MAP_SHARED and MAP_ANONYMOUS are default)
    (void) flags;
    (void) addr;
    (void) length;
    char fname[30];
    vfs_identify(CURRENT_TASK->resources[fd].f, fname, NULL, NULL);
    if (strcmp(fname, "fb0")) {
        printf("TODO: mmap currently can only map in the framebuffer device "
                "because why would you do things properly when you can just... "
                "not.\n");
        return NULL;
    }
    if (offset) {
        printf("TODO: offset currently must be 0 in mmap syscall\n");
        return NULL;
    }
    return kernel.framebuffer.addr;
}
