#pragma once
#include <stddef.h>

typedef size_t off_t;

#define MS_SYNC  0b01
#define MS_ASYNC 0b10

void* mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);
int msync(void *addr, size_t length, int flags);
