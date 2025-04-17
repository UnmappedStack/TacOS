#include <unistd.h>
#include <string.h>
#include <stdlib.h>
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

extern char **environ;
int execvp(const char *path, char **argv) {
    if (path[0] == '/')
        return execve(path, argv, environ);
    char *pathlist = getenv("PATH");
    if (!pathlist) {
        printf("execvp failed: $PATH environmental variable is not defined\n");
        return 1;
    }
    size_t start = 0;
    size_t end = 0;
    for (; !end || pathlist[end - 1]; end++) {
        if (pathlist[end] == ':' || !pathlist[end]) {
            char temp = pathlist[end];
            char *fullpath = (char*) malloc(strlen(&pathlist[start]) + strlen(path) + 1);
            sprintf(fullpath, "%s/%s", &pathlist[start], path);
            // check if the path exists by opening it, and if it does, try call execve on it
            int f;
            if ((f=open(fullpath, 0, 0)) >= 0) {
                close(f);
                execve(fullpath, argv, environ);
            }
            free(fullpath);
            pathlist[end] = temp;
            start = end = end + 1;
        }
    }
    printf("execvp failed: not found on $PATH\n");
    return -1;
}
