#include <syscall.h>
#include <unistd.h>

pid_t waitpid(pid_t pid, int *status, int options) {
    __syscall3(19, pid, status, (size_t) options);
}
