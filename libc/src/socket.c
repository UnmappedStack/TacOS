#include <sys/socket.h>
#include <stdint.h>
#include <syscall.h>

int socket(int domain, int type, int protocol) {
    return __syscall3(25, domain, type, protocol); 
}
