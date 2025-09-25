#include <ipc.h>
#include <printf.h>
#include <kernel.h>
#include <mem/slab.h>

void init_ipc(void) {
    kernel.ipc_socket_cache = init_slab_cache(sizeof(Socket), "IPC Sockets");
    kernel.ipc_socketqueueitem_cache = init_slab_cache(sizeof(SocketQueueItem), "IPC Socket Queue Item");
}

void ringbuffer_init(RingBuffer *rb) {
    rb->read_at = rb->write_at = rb->avaliable_to_read = 0;
}

int ringbuf_read(RingBuffer *rb, size_t len, char *buf) {
    for (size_t i = 0; i < len; i++) {
        if (!rb->avaliable_to_read) return i;
        buf[i] = rb->data[rb->read_at++];
        if (rb->read_at >= RINGBUF_MAX_LEN) rb->read_at = 0;
        rb->avaliable_to_read--;
    }
    return len;
}

int ringbuf_write(RingBuffer *rb, size_t len, char *buf) {
    for (size_t i = 0; i < len; i++) {
        rb->data[rb->write_at++] = buf[i];
        if (rb->write_at >= RINGBUF_MAX_LEN) rb->write_at = 0;
        rb->avaliable_to_read++;
    }
    return len;
}

int socket_read(Socket *file, char *buf, size_t len, size_t offset) {
    (void) offset;
    Socket *client = file;
    Socket *server = client->connected_to_server;
    if (!server) {
        printf("Not connected to a server\n");
        return -1;
    }
    int pid = kernel.scheduler.current_task->pid;
    RingBuffer *rb = (pid == server->owner_pid) ? &client->client_to_server_pipe : &client->server_to_client_pipe;
    return ringbuf_read(rb, len, buf);
}

int socket_write(Socket *file, char *buf, size_t len, size_t offset) {
    (void) offset;
    Socket *client = file;
    Socket *server = client->connected_to_server;
    if (!server) {
        printf("Client given is not connected to a server\n");
        return -1;
    }
    int pid = kernel.scheduler.current_task->pid;
    RingBuffer *rb = (pid == server->owner_pid) ? &client->server_to_client_pipe : &client->client_to_server_pipe;
    return ringbuf_write(rb, len, buf);
}

int sys_accept(int sockfd, struct sockaddr *addr,
                  socklen_t *addrlen, bool block) {
    if (addr) {
        printf("addr must be NULL, writing address is not supported yet in accept()\n");
        (void) addrlen;
        return -1;
    }
    VfsFile *srvfile = kernel.scheduler.current_task->resources[sockfd].f;
    Socket *server = srvfile->private;
    if (block)
        while (server->pending_queue.next == &server->pending_queue);
    else if (server->pending_queue.next == &server->pending_queue) return -1;
    SocketQueueItem *client = (SocketQueueItem*) server->pending_queue.next;
    list_remove(&client->list);
    list_insert(&server->connected_queue, &client->list);

    client->file->ops.read_fn  = (int (*)(void *file, char *buf, size_t len, size_t offset)) socket_read;
    client->file->ops.write_fn = (int (*)(void *file, char *buf, size_t len, size_t offset)) socket_write;

    // Open to a resource
    int fd = 0;
    for (size_t i = 0; i < MAX_RESOURCES; i++) {
        if (kernel.scheduler.current_task->resources[i].f) continue;
        kernel.scheduler.current_task->resources[i].f = client->file;
        kernel.scheduler.current_task->resources[i].offset = 0;
        fd = i;
        break;
    }
    return fd;
}

int sys_connect(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen) {
    (void) addrlen;
    if (addr->family != AF_UNIX) {
        printf("connect() only supports local unix sockets, invalid family\n");
        return -1;
    }
    VfsFile *f = vfs_access((char*) addr->data, 0, VAT_open);
    if (f == NULL) return -1;
    Socket *server_socket = ((TempfsInode*) f->private)->private;
    SocketQueueItem *newentry = (SocketQueueItem*) slab_alloc(kernel.ipc_socketqueueitem_cache);
    newentry->socket = kernel.scheduler.current_task->resources[sockfd].f->private;
    newentry->file = kernel.scheduler.current_task->resources[sockfd].f;

    list_insert(&server_socket->pending_queue, &newentry->list);
    newentry->socket->connected_to_server = server_socket;
    return 0;
}

int sys_listen(int sockfd, int backlog) {
    Socket *socket = kernel.scheduler.current_task->resources[sockfd].f->private;
    socket->listening = true;
    socket->backlog_max_len = backlog;
    return 0;
}

int sys_socket(int domain, int type, int protocol) {
    if (domain != AF_UNIX) {
        printf("Only Unix local sockets are supported, invalid domain\n");
        return -1;
    }
    if (type != SOCK_STREAM) {
        printf("Only SOCK_STREAM is supported, invalid socket type\n");
        return -1;
    }
    if (protocol) {
        printf("Only protocol=0 is supported, invalid protocol\n");
        return -1;
    }
    Socket *socket = (Socket*) slab_alloc(kernel.ipc_socket_cache);
    socket->connected_to_server = NULL;
    socket->owner_pid = kernel.scheduler.current_task->pid;
    ringbuffer_init(&socket->server_to_client_pipe);
    ringbuffer_init(&socket->client_to_server_pipe);
    list_init(&socket->pending_queue);
    list_init(&socket->connected_queue);
    VfsFile *file = slab_alloc(kernel.vfs_file_cache);
    file->ops = tempfs_regfile_ops; 
    file->private = socket;
    file->ops.read_fn  = (int (*)(void *file, char *buf, size_t len, size_t offset)) socket_read;
    file->ops.write_fn = (int (*)(void *file, char *buf, size_t len, size_t offset)) socket_write;
    int fd = -1;
    for (size_t i = 0; i < MAX_RESOURCES; i++) {
        if (kernel.scheduler.current_task->resources[i].f) continue;
        kernel.scheduler.current_task->resources[i].f = file;
        kernel.scheduler.current_task->resources[i].offset = 0;
        fd = i;
        break;
    }
    if (fd < 0) {
        printf("Too many resources open to open socket\n");
        return -1;
    }
    return fd;
}

int sys_bind(int sockfd, const struct sockaddr *addr,
                socklen_t addrlen) {
    (void) addrlen;
    if (addr->family != AF_UNIX) {
        printf("bind() only supports local unix sockets, invalid family\n");
        return -1;
    }
    VfsFile *f = vfs_access((char*) addr->data, 0, VAT_mkfile);
    if (f->drive.fs.fs_id != fs_tempfs) {
        printf("bind() only supports anchor creation on TempFS\n");
        return -1;
    }
    TempfsInode *private = f->private;
    private->ops = kernel.scheduler.current_task->resources[sockfd].f->ops;
    private->private = kernel.scheduler.current_task->resources[sockfd].f->private;
    memcpy(f, kernel.scheduler.current_task->resources[sockfd].f, sizeof(VfsFile));
    return 0;
}

