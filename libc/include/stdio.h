#pragma once
#include <stddef.h>
#include <stdarg.h>
#include <sprintf.h>
#include <stdint.h>
#include <stdbool.h>
#define ssize_t int64_t // TODO: Move these to their
#define mode_t uint32_t // relevant header files
#define BUFSIZ 8192
#define EOF -1

typedef size_t off_t;

#define getc fgetc

typedef enum {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END,
} FileLocs;

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
    bool eof, err;
    char *fname;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int putchar(int ch);
int open(const char *pathname, int flags, mode_t mode);
int remove(char *filename);
int rename(const char *oldpath, const char *newpath);
int close(int fd);
size_t write(int fd, const void *buf, size_t count);
int puts(char *str);
ssize_t read(int fd, void *buf, size_t count);
int printf(const char *fmt, ...);
int sscanf(const char *str, const char *format, ...);
int vfprintf(FILE *stream, const char *fmt, va_list args);
int fprintf(FILE *stream, const char *fmt, ...);
int putchar(int ch);

FILE* fopen(const char *restrict pathname, const char *restrict mode);
int fclose(FILE *stream);
size_t fwrite(const void *restrict ptr, size_t size, size_t nitems,
    FILE *restrict stream);
int fputs(const char *str, FILE *stream);
int fputc(int ch, FILE *stream);
char *fgets(char *str, int n, FILE *stream);
int fflush(FILE *stream);
int setvbuf(FILE *stream, char *buffer, int mode, size_t size);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
long ftell(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int fgetc(FILE *stream);
int getc(FILE *stream);
int getchar();
int fileno(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
FILE *freopen(const char *pathname, const char *mode, FILE *stream);
