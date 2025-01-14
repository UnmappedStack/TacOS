#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct HeapPool HeapPool;

struct HeapPool {
    uint8_t   verify;
    HeapPool *next;
    size_t    size;
    size_t    required_size;
    bool      free;
    uint8_t   data[0];
} __attribute__((packed));

void *malloc(size_t size);
void exit(int status);
