#include <syscall.h>
#include <stddef.h>
#include <pty.h>

int openpty(int *amaster, int *aslave, char *name, void *termp, void *winp) {
    return __syscall5(32, (size_t) amaster, (size_t) aslave,
            (size_t) name, (size_t) termp, (size_t) winp);
}
