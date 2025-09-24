#pragma once
#include <stdio.h>
#include <stddef.h>

typedef size_t off_t;

#define MS_SYNC  0b01
#define MS_ASYNC 0b10

#define MAP_ANONYMOUS 0b01
#define MAP_SHARED    0b10

void* mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);
int msync(void *addr, size_t length, int flags);

int shm_open(const char *name, int oflag, mode_t mode);
