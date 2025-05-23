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

int execl(const char *path, const char *arg0, ...) {
    char **argv = malloc(sizeof(char*));
    int argc = 1;
    argv[0] = path;
    va_list args;
    va_start(args, format);
    char *arg;
    while (arg=va_arg(args, char*)) {
        argv = realloc(argv, (++argc+1) * sizeof(char*));
        argv[argc - 1] = arg;
    }
    argv[argc] = NULL;
    va_end(args);
    return execve(path, argv, environ);
}

int execvp(const char *path, char **argv) {
    int contains_slash = strchr(path, '/') != NULL;
    if (path[0] == '/' || contains_slash)
        return execve(path, argv, environ);
    char *pathlist = getenv("PATH");
    if (!pathlist) return -1;
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
    return -1;
}

int chdir(const char *path) {
    return __syscall1(21, (size_t) path);
}

char *getcwd(char *buf, size_t size) {
    return (char*) __syscall2(20, (size_t) buf, size);
}
