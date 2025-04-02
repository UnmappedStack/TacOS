#define STB_SPRINTF_IMPLEMENTATION
#include <sprintf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syscall.h>
#include <fcntl.h>

#define stdout_fd 0

int open(const char *pathname, int flags, mode_t mode) {
    return __syscall3(2, (size_t) pathname, (size_t) flags, mode);
}

int close(int fd) {
    return __syscall1(3, fd);
}

size_t write(int fd, const void *buf, size_t count) {
    return __syscall3(1, (size_t) fd, (size_t) buf, count);
}

int puts(char *str) {
    write(stdout_fd, str, strlen(str));
    return 0;
}

int printf(const char *fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    int len = vsnprintf(NULL, 0, fmt, args) + 1;
    char *buf = (char*) malloc(len);
    int ret = vsnprintf(buf, len, fmt, copy);
    puts(buf);
    va_end(args);
    va_end(copy);
    return ret;
}

int putchar(int ch) {
    write(stdout_fd, &ch, 1);
    return ch;
}

int str_to_flags(const char *restrict mode) {
    int ret = 0;
    bool can_write = 0;
    bool can_read = 0;
    for (; *mode; mode++) {
        switch (*mode) {
            case 'w':
                ret |= O_CREAT;
                can_write = 1;
                break;
            case 'r':
                can_read = 1;
                break;
            default:
                printf("Invalid flag when opening file!\n");
                exit(1);
        }
    }
    if (can_write && !can_read) ret |= O_WRONLY;
    else if (!can_write && can_read) ret |= O_RDONLY;
    else if (can_write && can_write) ret |= O_RDWR;
    return ret;
}

FILE* fopen(const char *restrict pathname, const char *restrict mode) {
    int flags = str_to_flags(mode);
    FILE *ret = (FILE*) malloc(sizeof(FILE));
    ret->fd = open(pathname, flags, 0);
    ret->buffer = (char*) malloc(4096);
    return ret;
}

int fclose(FILE *stream) {
    if (close(stream->fd)) return -1;
    free(stream->buffer);
    free(stream);
    return 0;
}
