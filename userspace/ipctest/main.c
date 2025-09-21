#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

#define PATH "/testsocket"

int main(int argc, char **argv) {
    printf(" > Trying to open server socket...\n");
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
err:
        printf("failed whoopsie\n");
        return -1;
    }
    printf("Success! Resource FD = %d.\n", fd);
    printf(" > Trying to bind to path...\n");
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, PATH);
    int s = bind(fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un));
    if (s < 0) goto err;
    printf("Successfully bound to %s\n", PATH);
}
