#include <ipc.h>
#include <printf.h>
#include <kernel.h>
#include <mem/slab.h>

void init_ipc(void) {
    kernel.ipc_socket_cache = init_slab_cache(sizeof(Socket), "IPC Sockets");
    kernel.ipc_socketqueueitem_cache = init_slab_cache(sizeof(SocketQueueItem), "IPC Socket Queue Item");
}

int sys_connect(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen) {
    (void) addrlen;
    if (addr->family != AF_UNIX) {
        printf("connect() only supports local unix sockets, invalid family\n");
        return -1;
    }
    VfsFile *f = vfs_access((char*) addr->data, 0, VAT_open);
    Socket *server_socket = ((TempfsInode*) f->private)->private;
    SocketQueueItem *newentry = (SocketQueueItem*) slab_alloc(kernel.ipc_socketqueueitem_cache);
    newentry->socket = kernel.scheduler.current_task->resources[sockfd].f->private;

    list_insert(&server_socket->pending_queue, &newentry->list);
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
    list_init(&socket->pending_queue);
    list_init(&socket->connected_queue);
    VfsFile *file = slab_alloc(kernel.vfs_file_cache);
    file->ops = tempfs_regfile_ops; // TODO: this needs to have separate read/write ops
    file->private = socket;
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

