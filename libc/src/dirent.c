#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <syscall.h>
#include <stddef.h>

DIR *opendir(char *path) {
    char buf[30];
    strncpy(buf, path, 30);
    size_t len = strlen(buf);
    if (buf[len - 1] == '/' && len > 1)
        buf[len - 1] = 0;
    path = buf;
    int fd;
    void *iterbuf = malloc(64); // assume iterbuf takes 64 bytes or less
    if ((fd=__syscall2(22, (size_t) iterbuf, (size_t) path)) < 0) return NULL;
    return iterbuf;
}

struct dirent *readdir(DIR *dirp) {
    struct dirent *ret = (struct dirent*) malloc(sizeof(struct dirent));
    if (__syscall2(23, (size_t) dirp, (size_t) ret)) { // syscall 23 = diriter
        free(ret);
        return NULL;
    }
    return ret;
}
