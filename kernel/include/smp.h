#pragma once
#include <scheduler.h>
#include <mem/paging.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define KERNEL_STACK_PAGES 10LL
#define MAX_CORES 16

#define CURRENT_TASK (cores[get_current_processor()].current_task)

typedef struct {
    uint8_t stack[PAGE_SIZE * KERNEL_STACK_PAGES] __attribute__((aligned(PAGE_SIZE)));
    Task *current_task;
} CPUCore;
extern CPUCore cores[MAX_CORES];

void init_smp(void);
