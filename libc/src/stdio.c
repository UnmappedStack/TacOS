#define STB_SPRINTF_IMPLEMENTATION
#include <sprintf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syscall.h>
#include <fcntl.h>

FILE *stdout;

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
    fputs(buf, stdout);
    va_end(args);
    va_end(copy);
    return ret;
}

int putchar(int ch) {
    write(stdout_fd, &ch, 1);
    return ch;
}

static int str_to_flags(const char *restrict mode) {
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
    ret->bufsz = 0;
    ret->bufmode = _IOFBF;
    return ret;
}

int fclose(FILE *stream) {
    if (stream->bufsz) {
        write(stream->fd, stream->buffer, stream->bufsz);
    }
    if (close(stream->fd)) return -1;
    free(stream->buffer);
    free(stream);
    return 0;
}

static bool char_in_mem(const char *ptr, char ch, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (ptr[i] == ch) return true;
    }
    return false;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, 
        FILE *restrict stream) {
    size_t bytes = size * nitems;
    switch (stream->bufmode) {
        case _IOFBF:
            // Full buffering
            if (stream->bufsz + bytes >= 4096) {
                write(stream->fd, stream->buffer, stream->bufsz);
                stream->bufsz = 0;
                return write(stream->fd, ptr, bytes);
            } else {
                memcpy(&stream->buffer[stream->bufsz], ptr, bytes);
                stream->bufsz += bytes;
                return bytes;
            }
        case _IOLBF:
            // Line buffering
            if (char_in_mem(ptr, '\n', bytes)) {
                stream->bufsz = 0;
                return write(stream->fd, ptr, bytes);
            } else {
                memcpy(&stream->buffer[stream->bufsz], ptr, bytes);
                stream->bufsz += bytes;
                return bytes;
            }
        case _IONBF:
            // No buffering
            return write(stream->fd, ptr, bytes);
        default:
            puts("Unknown buffer mode in fwrite (libc).\n");
            exit(0);
    }
}

int fputs(const char *str, FILE *stream) {
    fwrite(str, strlen(str), 1, stream);
    return 0;
}

int fflush(FILE *stream) {
    write(stream->fd, stream->buffer, stream->bufsz);
    return 0;
}
