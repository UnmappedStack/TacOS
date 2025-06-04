#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>

void *atexit_fns[32] = {0};
size_t natexit_fns = 0;
int is_init = 0;
char **environ;

__attribute__((noreturn))
void exit(int status) {
    for (size_t i = 0; i < natexit_fns; i++)
        ((void (*)(void)) atexit_fns[i])();
    fflush(stdout);
    __syscall1(4, status);
    for (;;);
}

int atexit(void* fn) {
    atexit_fns[natexit_fns++] = fn;
    return 0;
}

double atof(const char *nptr) {
    printf("TODO: atof() is not yet implemented because SSE2 is not supported in TacOS.\n");
    exit(1);
}

int atoi(const char *nptr) {
    int sign = 0; // set to 1 if negative
    while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n') nptr++;
    if (*nptr == '-') sign = 1, nptr++;
    else if (*nptr == '+') nptr++;
    else if (*nptr < '0' || *nptr > '9') return 0;
    int ret = 0;
    while (*nptr >= '0' && *nptr <= '9') {
        ret = ret * 10 + (*nptr - '0'); 
        nptr++;
    }
    if (sign) return -ret;
    else return ret;
}

// TODO: make this actually do signal stuff
__attribute__((noreturn))
void abort(void) {
    exit(-1);
}

int system(const char *command) {
    pid_t pid = fork();
    if (pid) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        execve("/usr/bin/shell", (char*[]) {"shell", "-c", command, NULL}, environ);
        exit(-1);
    }
    return 0;
}

size_t num_envp = 0;
void init_environ(char **envp) {
    for (; envp[num_envp]; num_envp++);
    num_envp++;
    environ = (char**) malloc(sizeof(char*) * num_envp);
    memcpy(environ, envp, sizeof(char*) * num_envp);
}

int get_env_idx(char *key, size_t key_len) {
    for (size_t i = 0; environ[i]; i++) {
        if (!memcmp(environ[i], key, key_len - 1)) return i;
    }
    return -1;
}

char *getenv(char *key) {
    size_t key_len = strlen(key);
    int idx = get_env_idx(key, key_len);
    return (idx < 0) ? NULL : &environ[idx][key_len + 1];
}

int setenv(char *key, char *val, int overwrite) {
    size_t key_len = strlen(key);
    int idx = get_env_idx(key, key_len);
    if (idx >= 0 && !overwrite) return 0;
    else if (idx < 0) {
        environ = (char**) realloc(environ, ++num_envp * sizeof(char*));
        idx = num_envp - 2;
        environ[idx + 1] = 0;
    }
    size_t val_len = strlen(val) + 1;
    environ[idx] = (char*) malloc(val_len + strlen(key) + 1);
    sprintf(environ[idx], "%s=%s", key, val);
    return 0;
}

int abs(int j) {
    return (j < 0) ? -j : j;
}

long strtol(const char *nptr,
                   char **endptr, int base) {
    (void) nptr, (void) endptr, (void) base;
    printf("TODO: strtol\n");
    return 0;
}

long __isoc23_strtol(const char *nptr,
                   char **endptr, int base) {
    return strtol(nptr, endptr, base);
}
