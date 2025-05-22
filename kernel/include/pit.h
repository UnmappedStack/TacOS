#pragma once
#include <stddef.h>
#include <stdint.h>

#define HERTZ_DIVIDER 1190

// for the global timer
struct timespec {
    size_t tv_sec;
    size_t tv_nsec;
};

void init_pit(void);
void lock_pit(void);
void unlock_pit(void);
void pit_wait(uint64_t ms);
