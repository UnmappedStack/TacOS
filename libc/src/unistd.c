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
    int contains_slash = strchr(path, '/') != NULL;
    if (contains_slash)
        return execve(path, argv, environ);
    char *pathlist = getenv("PATH");
    if (!pathlist) return -1;
    while (*pathlist) {
        char* start = pathlist;
        while (*pathlist && *pathlist != ':') pathlist++;
        char* dir = strndup(start, pathlist-start);
        char* pattern = "%s/%s";
        size_t len = sprintf(NULL, pattern, dir, path);
        char* full_path = (char*) malloc(len + 1);
        sprintf(full_path, pattern, dir, path);
        int f = open(full_path, 0, 0);
        if (f < 0) {
            fprintf(stderr, "failed to open from PATH %s (execvp)\n", full_path);
            free(dir);
            return -1;
        }
        execve(full_path, argv, environ);
        free(dir);
        if (!*pathlist) break;
        pathlist++;
    }
    return -1;
}

int chdir(const char *path) {
    return __syscall1(21, (size_t) path);
}

char *getcwd(char *buf, size_t size) {
    return (char*) __syscall2(20, (size_t) buf, size);
}
