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

int puts(char *str) {
    write(0, str, strlen(str));
    return 0;
}

int printf(const char *fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    int len = vsnprintf(NULL, 1000, fmt, args);
    char *buf = (char*) malloc(len + 1);
    int ret = vsnprintf(buf, len, fmt, copy);
    puts(buf);
    va_end(args);
    va_end(copy);
    free(buf);
    return ret;
}
