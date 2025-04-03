#include <time.h>
#include <syscall.h>

int clock_gettime(ClockID clockid, struct timespec *tp) {
    return __syscall2(16, clockid, (size_t) tp);
}
