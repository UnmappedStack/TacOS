#pragma once

#define AF_UNIX 0
#define AF_LOCAL AF_UNIX

#define SOCK_STREAM 0

typedef int socklen_t;
typedef int sa_family_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[20];
};

struct sockaddr_un {
    sa_family_t sun_family;
    char sun_path[20];
};

int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr,
                socklen_t addrlen);
int listen(int sockfd, int backlog);
