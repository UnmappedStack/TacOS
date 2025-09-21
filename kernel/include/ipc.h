#pragma once

#define AF_UNIX 0
#define AF_LOCAL AF_UNIX

#define SOCK_STREAM 0

// This will be referenced in task resource lists
// and anchor points in tempfs file lists
typedef struct {
    char tag[20]; // tag of this socket to be used for testing mostly
} Socket;

void init_ipc(void);
