#pragma once
#include <stddef.h>
#include <stdint.h>

#define HERTZ_DIVIDER 1190

// for the global timer
struct timespec {
    size_t tv_sec;
    size_t tv_nsec;
};

void init_pit();
void lock_pit();
void unlock_pit();
void pit_wait(uint64_t ms);
