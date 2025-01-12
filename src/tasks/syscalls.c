#include <exec.h>
#include <fork.h>
#include <cpu.h>
#include <kernel.h>
#include <scheduler.h>
#include <stdint.h>
#include <printf.h>

int sys_open(char *filename, int flags, int mode) {
    (void) mode;
    uint64_t file_descriptor = 0;
    Task *current_task = kernel.scheduler.current_task;
    for (; file_descriptor < MAX_RESOURCES; file_descriptor++) {
        if (current_task->resources[file_descriptor]) continue;
        current_task->resources[file_descriptor] = open((char*) filename, flags);
        return file_descriptor;
    }
    return -1;
}

int sys_close(int fd) {
    return close(kernel.scheduler.current_task->resources[fd]);
}

int sys_read(int fd, char *buf, size_t count) {
    return vfs_read(kernel.scheduler.current_task->resources[fd], buf, count, 0);
}

int sys_write(int fd, char *buf, size_t count) {
    printf("fd = %i, buf = %s, count = %i\n", fd, buf, count);
    return vfs_write(kernel.scheduler.current_task->resources[fd], buf, count, 0);
}

void sys_exit(int status) {
    // TODO: Clean up and report to parent
    if (kernel.scheduler.current_task->pid <= 1) {
        printf("Init task exited! Halting device.\n");
        HALT_DEVICE();
    }
    printf("Exited task with status %i\n", status);
    kernel.scheduler.current_task->flags &= ~TASK_PRESENT;
    kernel.scheduler.current_task->flags |= TASK_DEAD;
    for (;;);
}

int sys_getpid() {
    return kernel.scheduler.current_task->pid;
}

int sys_fork() {
    return fork();
}

int sys_execve(char *path) {
    return execve(kernel.scheduler.current_task, path);
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
        printf("Killed task %i with signal %i\n", pid, sig);
        return 0;
    }
}

int sys_isatty(int fd) {
    VfsFile *f = kernel.scheduler.current_task->resources[fd];
    if (!f) return 0;
    if (f->drive.fs.fs_id == fs_tempfs) {
        TempfsInode *private = f->private;
        return private->type == Device && private->devops.is_term;
    }
    return 0;
}

void sys_invalid(int sys) {
    printf("Invalid syscall: %i\n", sys);
}
