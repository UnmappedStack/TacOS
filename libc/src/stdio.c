#include <stdio.h>
#include <syscall.h>

int open(const char *pathname, int flags, mode_t mode) {
    return __syscall3(2, (size_t) pathname, flags, mode);
}

size_t write(int fd, const void *buf, size_t count) {
    return __syscall3(1, (size_t) fd, buf, count);
}
