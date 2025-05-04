#pragma once
#include <stddef.h>

typedef long time_t;

struct timespec {
    size_t tv_sec;
    size_t tv_nsec;
};

typedef enum {
    CLOCK_REALTIME,
} ClockID;

int clock_gettime(ClockID clockid, struct timespec *tp);
int nanosleep(struct timespec *duration);
time_t time(time_t *tloc);
