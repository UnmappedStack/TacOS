#include <syscall.h>

int sched_yield(void) {
    __syscall0(17);
}
