#define STB_SPRINTF_IMPLEMENTATION
#include <stdbool.h>
#include <sprintf.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syscall.h>
#include <fcntl.h>

FILE *stdin;
FILE *stdout;
FILE *stderr;

void init_streams(void) {
    stdin = (FILE*) malloc(sizeof(FILE));
    stdin->fd = 0;
    stdin->bufmode = _IONBF;

    stdout = (FILE*) malloc(sizeof(FILE));
    stdout->fd = 1;
    stdout->buffer = (char*) malloc(512);
    stdout->bufsz = 0;
    stdout->bufmax = 512;
    stdout->bufmode = _IOLBF;

    stderr = (FILE*) malloc(sizeof(FILE));
    stderr->fd = 2;
    stderr->bufmode = _IONBF;
}

int remove(char *filename) {
    return __syscall1(13, (size_t) filename);
}

int rename(const char *oldpath, const char *newpath) {
    printf("TODO: rename() in libc\n");
    return 0;
}

int open(const char *pathname, int flags, mode_t mode) {
     return __syscall3(2, (size_t) pathname, (size_t) flags, mode);
}

int close(int fd) {
    return __syscall1(3, fd);
}

size_t write(int fd, const void *buf, size_t count) {
    return __syscall3(1, (size_t) fd, (size_t) buf, count);
}

ssize_t read(int fd, void *buf, size_t count) {
    return __syscall3(0, (size_t) fd, (size_t) buf, count);
}

int puts(char *str) {
    write(stdout->fd, str, strlen(str));
    putchar('\n');
    fflush(stdout);
    return 0;
}

int vfprintf(FILE *stream, const char *fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int len = vsnprintf(NULL, 0, fmt, args) + 1;
    char *buf = (char*) malloc(len);
    int ret = vsnprintf(buf, len, fmt, copy);
    fputs(buf, stream);
    free(buf);
    va_end(copy);
    return ret;
}
int set = 0;
int printf(const char *fmt, ...) {
    set = 1;
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stdout, fmt, args);
    va_end(args);
    return ret;
}

int fprintf(FILE *stream, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stream, fmt, args);
    va_end(args);
    return ret;
}

int putchar(int ch) {
    write(stdout->fd, &ch, 1);
    fflush(stdout);
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
            case 'b':
                break;
            default:
                printf("TODO: Invalid flag when opening file! Flags: %s\n", mode);
                break;
        }
    }
    if (can_write && !can_read) ret |= O_WRONLY;
    else if (!can_write && can_read) ret |= O_RDONLY;
    else if (can_write && can_write) ret |= O_RDWR;
    return ret;
}

int __fopen_internal(const char *restrict pathname, const char *restrict mode, FILE *ret) {
    ret->fname = (char*) malloc(strlen(pathname) + 1);
    strcpy(ret->fname, pathname);
    int flags = str_to_flags(mode);
    ret->fd = open(pathname, flags, 0);
    if (ret->fd < 0) return -1;
    ret->bufmax = 4096;
    ret->buffer = (char*) malloc(ret->bufmax);
    ret->bufsz = 0;
    ret->bufmode = _IOFBF;
    ret->err=ret->eof=false;
    return 0;
}

FILE *fopen(const char *restrict pathname, const char *restrict mode) {
    FILE *ret = (FILE*) malloc(sizeof(FILE));
    if (__fopen_internal(pathname, mode, ret) < 0) {
        free(ret);
        return NULL;
    }
    return ret;
}

int fclose(FILE *stream) {
    if (stream->bufsz) {
        write(stream->fd, stream->buffer, stream->bufsz);
    }
    if (close(stream->fd)) return -1;
    free(stream->buffer);
    free(stream->fname);
    free(stream);
    return 0;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, 
        FILE *restrict stream) {
    size_t bytes = size * nitems;
    switch (stream->bufmode) {
        case _IOFBF:
            // Full buffering
            if (stream->bufsz + bytes >= stream->bufmax) {
                write(stream->fd, stream->buffer, stream->bufsz);
                stream->bufsz = 0;
                return write(stream->fd, ptr, bytes) / size;
            } else {
                memcpy(&stream->buffer[stream->bufsz], ptr, bytes);
                stream->bufsz += bytes;
                return bytes / size;
            }
        case _IOLBF:
            // Line buffering
            if (memchr(ptr, '\n', bytes)) {
                memcpy(&stream->buffer[stream->bufsz], ptr, bytes);
                stream->bufsz += bytes;
                size_t ret = write(stream->fd, stream->buffer, stream->bufsz);
                stream->bufsz = 0;
                return ret / size;
            } else {
                memcpy(&stream->buffer[stream->bufsz], ptr, bytes);
                stream->bufsz += bytes;
                return bytes / size;
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

char *fgets(char *str, int n, FILE *stream) {
    size_t bytes;
    if ((bytes=read(stream->fd, str, n)) < n) {
        // we'll assume this means end of file for now. Probably not the smartest assumption to make.
        stream->eof = true;
    }
    str[bytes] = 0;
    return str;
}

int fflush(FILE *stream) {
    write(stream->fd, stream->buffer, stream->bufsz);
    stream->bufsz = 0;
    return 0;
}

int setvbuf(FILE *stream, char *buffer, int mode, size_t size) {
    if (buffer) {
        stream->buffer = buffer;
        stream->bufsz = size;
    }
    stream->bufmode = mode;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (!size) return 0;
    size_t bytes;
    if ((bytes=read(stream->fd, ptr, size * nmemb)) < size * nmemb) {
        // we'll assume this means end of file for now. Probably not the smartest assumption to make.
        stream->eof = true;
    }
    return bytes / size;
}

int sscanf(const char *str, const char *format, ...) {
    printf("TODO: sscanf is not implemented yet\n");
    exit(1);
}

int __isoc99_sscanf(const char *str, const char *format, ...) {
    printf("TODO: __isoc99_sscanf is not implemented yet\n");
    exit(1);
}

long ftell(FILE *stream) {
    return lseek(stream->fd, 0, SEEK_CUR);
}

int fseek(FILE *stream, long offset, int whence) {
    lseek(stream->fd, offset, whence);
    return 0;
}

int fputc(int ch, FILE *stream) {
    fwrite(&ch, 1, 1, stream);
    return ch;
}

int fgetc(FILE *stream) {
    char ret;
    fread(&ret, 1, 1, stream);
    return (ret) ? (int) ret : EOF;
}

int getchar() {
    return getc(stdin);
}

int fileno(FILE *stream) {
    return stream->fd;
}

int feof(FILE *stream) {
    return stream->eof;
}

int ferror(FILE *stream) {
    return stream->err;
}

FILE *freopen(const char *pathname, const char *mode, FILE *stream) {
    fflush(stream);
    close(stream->fd);
    char *fname = (pathname) ? pathname : stream->fname;
    __fopen_internal(fname, mode, stream);
    return stream;
}
