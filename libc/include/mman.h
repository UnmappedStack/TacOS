#pragma once
#include <stddef.h>

typedef size_t off_t;

void* mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);
