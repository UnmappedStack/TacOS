#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int main(int argc, char **argv) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
err:
        fprintf(stderr, "Failed to connect to window server\n");
        return -1;
    }
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/winsrv");
    if (connect(fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) goto err;
    printf("Successfully connected to window server\n");
}
