#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syscall.h>

int open(const char *pathname, int flags, mode_t mode) {
    return __syscall3(2, (size_t) pathname, (size_t) flags, mode);
}

size_t write(int fd, const void *buf, size_t count) {
    return __syscall3(1, (size_t) fd, (size_t) buf, count);
}
