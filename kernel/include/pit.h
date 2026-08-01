#pragma once
#include <stddef.h>
#include <stdint.h>

#define HERTZ_DIVIDER 1190

void pit_init(void);
void lock_pit(void);
void unlock_pit(void);
void pit_wait(uint64_t ms);
