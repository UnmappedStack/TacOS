#pragma once
#include <mem/paging.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define KERNEL_STACK_PAGES 10LL
#define MAX_CORES 16
typedef struct {
    uint8_t stack[PAGE_SIZE * KERNEL_STACK_PAGES] __attribute__((aligned(PAGE_SIZE)));
} CPUCore;
extern CPUCore cores[MAX_CORES];

void init_smp(void);
