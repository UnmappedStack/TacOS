#pragma once
#include <stddef.h>

struct timespec {
    size_t tv_sec;
    size_t tv_nsec;
};

typedef enum {
    CLOCK_REALTIME,
} ClockID;

int clock_gettime(ClockID clockid, struct timespec *tp);
