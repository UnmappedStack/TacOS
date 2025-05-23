#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef int pid_t;
typedef size_t off_t;

void *sbrk(intptr_t increment);
off_t lseek(int fd, size_t offset, int whence);
pid_t fork(void);
int execve(const char *path, char **argv, char **envp);
int execvp(const char *path, char **argv);
int execl(const char *path, const char *arg0, ...);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
