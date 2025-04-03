#include <sys/stat.h>
#include <syscall.h>

int mkdir(char *path, mode_t mode) {
    return __syscall2(14, (size_t) path, mode);
}
