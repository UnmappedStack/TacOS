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

void sys_invalid(int sys) {
    printf("Invalid syscall: %i\n", sys);
}
