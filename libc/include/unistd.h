#pragma once
#include <stdint.h>
#include <stddef.h>

typedef size_t off_t;

void *sbrk(intptr_t increment);
off_t lseek(int fd, size_t offset, int whence);
