#pragma once
#include <stddef.h>
#include <stdarg.h>
#include <sprintf.h>
#define ssize_t long long   // TODO: Move these to their
#define mode_t unsigned int // relevant header files

typedef enum {
    _IOFBF, // buffer when full
    _IOLBF, // buffer on newline char
    _IONBF, // no buffer (write immediately)
} BufMode;

typedef struct {
    int fd;
    char *buffer;
    size_t bufsz;
    size_t bufmax;
    BufMode bufmode;
} FILE;

extern FILE *stdout;

int putchar(int ch);
int open(const char *pathname, int flags, mode_t mode);
int close(int fd);
size_t write(int fd, const void *buf, size_t count);
int puts(char *str);
ssize_t read(int fd, void *buf, size_t count);
int printf(const char *fmt, ...);
int vfprintf(FILE *stream, const char *fmt, va_list args);
int fprintf(FILE *stream, const char *fmt, ...);
int putchar(int ch);

FILE* fopen(const char *restrict pathname, const char *restrict mode);
int fclose(FILE *stream);
size_t fwrite(const void *restrict ptr, size_t size, size_t nitems,
    FILE *restrict stream);
int fputs(const char *str, FILE *stream);
int fflush(FILE *stream);
int setvbuf(FILE *stream, char *buffer, int mode, size_t size);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
