#pragma once
#include <stddef.h>
#define ssize_t long long   // TODO: Move these to their
#define mode_t unsigned int // relevant header files

int open(const char *pathname, int flags, mode_t mode);
size_t write(int fd, const void *buf, size_t count);
