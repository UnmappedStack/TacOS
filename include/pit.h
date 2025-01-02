#pragma once
#include <pic.h>

#define HERTZ_DIVIDER 1190

void init_pit();
void lock_pit();
void unlock_pit();
