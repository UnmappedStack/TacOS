#pragma once
#include <scheduler.h>

// GSBase is used to store a pointer to the CPU struct for the current processor
#define GSBASE 0xC0000101

/* All information and status stuff related to one CPU. */
typedef struct {
    uint64_t id;
    ProcessorQueue *scheduler;
} CPU;

void smp_init(void);
CPU *current_processor(void);
