#include <time.h>
#include <sched.h>
#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>

int clock_gettime(ClockID clockid, struct timespec *tp) {
    return __syscall2(16, clockid, (size_t) tp);
}

int nanosleep(struct timespec *duration) {
    const size_t ns_in_s = 1000000000;
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    struct timespec finish_time;
    finish_time.tv_sec = current_time.tv_sec + duration->tv_sec;
    finish_time.tv_nsec = current_time.tv_nsec + duration->tv_nsec;
    if (finish_time.tv_nsec > ns_in_s) {
        size_t secs_in_nsec = finish_time.tv_nsec % ns_in_s;
        finish_time.tv_sec += secs_in_nsec;
        finish_time.tv_nsec -= secs_in_nsec * ns_in_s;
    }
    while (!(current_time.tv_sec > finish_time.tv_sec ||
                (current_time.tv_sec == finish_time.tv_sec &&
                 current_time.tv_nsec >= finish_time.tv_nsec))) {
        clock_gettime(CLOCK_REALTIME, &current_time);
        sched_yield();
    }
    return 0;
}

time_t time(time_t *tloc) {
    (void) tloc;
    printf("TODO: time() is not yet implemented in LibC\n");
    exit(-1);
}
