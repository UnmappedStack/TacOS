#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

#define PATH "/testsocket"

int server(void) {
    printf(" > Trying to open server socket...\n");
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
err:
        printf("There was an error (server)\n");
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
    printf(" > Starting to listen for connect requests...\n");
    if (listen(fd, 10) < 0) goto err;
    printf("Successfully started listening.\n");
    printf("Starting client...\n");
    int pid = fork();
    if (!pid) execve("/usr/bin/ipctest", (char*[]){"ipctest", "client", NULL}, (char*[]){NULL});
    return 0;
}

int client(void) {
    printf(" > Trying to open client socket...\n");
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
err:
        printf("There was an error (client)\n");
        return -1;
    }
    printf("Success! Resource FD = %d.\n", fd);
    printf(" > Client trying to connect to server...\n");
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, PATH);
    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) goto err;
    printf("Successfully sent connect() request, waiting for server to respond...\n");
}

int main(int argc, char **argv) {
    int status = (argc > 1) ? client() : server();
    return status;
}
