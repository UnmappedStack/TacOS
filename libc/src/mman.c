#include <syscall.h>
#include <stdint.h>
#include <sys/mman.h>

void* mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset) {
    return (void*) __syscall6(18, (size_t) addr, length, (size_t) prot, (size_t) flags, (size_t) fd, offset);
}

int msync(void *addr, size_t length, int flags) {
    return __syscall3(30, addr, length, flags);
}
