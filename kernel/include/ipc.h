#pragma once
#include <stdbool.h>

#define AF_UNIX 0
#define AF_LOCAL AF_UNIX

#define SOCK_STREAM 0

typedef int socklen_t;

// This will be referenced in task resource lists
// and anchor points in tempfs file lists
typedef struct {
    bool listening;
    int backlog_max_len;
} Socket;

struct sockaddr {
    int family;
    char data[20];
}; 

void init_ipc(void);
