#include <syscall.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>

void* mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset) {
    return (void*) __syscall6(18, (size_t) addr, length, (size_t) prot, (size_t) flags, (size_t) fd, offset);
}

int msync(void *addr, size_t length, int flags) {
    return __syscall3(30, addr, length, flags);
}

int shm_open(const char *name, int oflag, mode_t mode) {
    mkdir("/dev/shm", 0); // it doesn't matter if it fails cos from existing already as we
                          // just needs to create it IF it doesn't exist already
    size_t buflen = strlen(name) + strlen("/dev/shm/") + 1;
    char *buf = (char*) malloc(buflen);
    sprintf(buf, "/dev/shm/%s", name);
    int fd = open(buf, oflag, mode);
    free(buf);
    return fd;
}
