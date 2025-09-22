#include <sys/socket.h>
#include <stddef.h>
#include <stdint.h>
#include <syscall.h>

int socket(int domain, int type, int protocol) {
    return __syscall3(25, domain, type, protocol); 
}

int bind(int sockfd, const struct sockaddr *addr,
                socklen_t addrlen) {
    return __syscall3(26, sockfd, addr, addrlen);
}

int listen(int sockfd, int backlog) {
    return __syscall2(27, sockfd, backlog);
}

int connect(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen) {
    return __syscall3(28, sockfd, addr, addrlen);
}

int accept_b(int sockfd, struct sockaddr *addr,
                  socklen_t *addrlen, bool block) {
    return __syscall4(29, sockfd, (size_t) addr, (size_t) addrlen, block);
}

int accept(int sockfd, struct sockaddr *addr,
                  socklen_t *addrlen) {
    return accept_b(sockfd, addr, addrlen, true);
}
