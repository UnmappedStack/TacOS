#pragma once
#include <stddef.h>
#include <unistd.h>

void* mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);
