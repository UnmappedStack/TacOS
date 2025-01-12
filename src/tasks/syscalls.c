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
        if (current_task->resources[file_descriptor]) continue;
        current_task->resources[file_descriptor] = open((char*) filename, flags);
        return file_descriptor;
    }
    return -1;
}

int sys_close(int fd) {
    return close(CURRENT_TASK->resources[fd]);
}

int sys_read(int fd, char *buf, size_t count) {
    return vfs_read(CURRENT_TASK->resources[fd], buf, count, 0);
}

int sys_write(int fd, char *buf, size_t count) {
    printf("fd = %i, buf = %s, count = %i\n", fd, buf, count);
    return vfs_write(CURRENT_TASK->resources[fd], buf, count, 0);
}

void sys_exit(int status) {
    // TODO: Clean up and report to parent
    if (CURRENT_TASK->pid <= 1) {
        printf("Init task exited! Halting device.\n");
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

int sys_fork() {
    return fork();
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
    VfsFile *f = CURRENT_TASK->resources[fd];
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
    if (!increment) return CURRENT_TASK->program_break;
    uintptr_t previous_break = CURRENT_TASK->program_break;
    CURRENT_TASK->program_break += increment;
    if (!(CURRENT_TASK->program_break % PAGE_SIZE) ||
         (PAGE_ALIGN_DOWN(CURRENT_TASK->program_break) > PAGE_ALIGN_DOWN(previous_break))) {
        size_t num_new_pages = PAGE_ALIGN_UP(increment) / 4096;
        alloc_pages((uint64_t*) (CURRENT_TASK->pml4 + kernel.hhdm), CURRENT_TASK->program_break, num_new_pages, KERNEL_PFLAG_WRITE | KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_USER);
        add_memregion(&CURRENT_TASK->memregion_list, CURRENT_TASK->program_break, num_new_pages, true, KERNEL_PFLAG_WRITE | KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_USER);
    }
    return previous_break;
}

void sys_invalid(int sys) {
    printf("Invalid syscall: %i\n", sys);
}
