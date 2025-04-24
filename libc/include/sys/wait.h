#pragma once
#include <unistd.h>

pid_t waitpid(pid_t pid, int *status, int options);
