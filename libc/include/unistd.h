#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef int pid_t;

void *sbrk(intptr_t increment);
off_t lseek(int fd, size_t offset, int whence);
pid_t fork(void);
int execve(const char *path, char **argv, char **envp);
int execvp(const char *path, char **argv);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
