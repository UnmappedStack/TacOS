#pragma once
#include <fs/vfs.h>
#include <list.h>
#include <stdbool.h>

#define AF_UNIX 0
#define AF_LOCAL AF_UNIX

#define SOCK_STREAM 0

typedef int socklen_t;

#define RINGBUF_MAX_LEN 4096 * 1000
typedef struct {
    char data[RINGBUF_MAX_LEN];
    size_t read_at;
    size_t write_at;
    size_t avaliable_to_read;
} RingBuffer;

// This will be referenced in task resource lists
// and anchor points in tempfs file lists
typedef struct Socket Socket;
struct Socket {
    int owner_pid;
    Socket *connected_to_server; // NULL if this is a server or it's not connected
    bool listening;
    int backlog_max_len;
    struct list pending_queue;
    struct list connected_queue;
    RingBuffer server_to_client_pipe;
    RingBuffer client_to_server_pipe;;
};

typedef struct {
    struct list list;
    Socket *socket;
    VfsFile *file; // to open as a resource
} SocketQueueItem;

struct sockaddr {
    int family;
    char data[20];
}; 

void init_ipc(void);
