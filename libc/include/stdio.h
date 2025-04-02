#pragma once
#include <stddef.h>
#include <stdarg.h>
#include <sprintf.h>
#define ssize_t long long   // TODO: Move these to their
#define mode_t unsigned int // relevant header files

int putchar(int ch);
int open(const char *pathname, int flags, mode_t mode);
size_t write(int fd, const void *buf, size_t count);
int puts(char *str);
int printf(const char *fmt, ...);
int putchar(int ch);
