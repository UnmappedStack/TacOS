#include <unistd.h>
#include <stdio.h>
#include <syscall.h>

#if UINT32_MAX == UINTPTR_MAX
    #define STACK_CHK_GUARD 0xe2dee396
#else
    #define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
__attribute__((noreturn))
void __stack_chk_fail(void) {
    for (;;); // TODO: Handle properly
}

void *sbrk(intptr_t increment) {
    return (void*) __syscall1(11, increment);
}

off_t lseek(int fd, size_t offset, int whence) {
    return __syscall3(15, fd, offset, whence);
}

pid_t fork(void) {
    return __syscall0(6);
}

int execve(const char *path, char **argv, char **envp) {
    return __syscall3(7, (size_t) path, (size_t) argv, (size_t) envp);
}
