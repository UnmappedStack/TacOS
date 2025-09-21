#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int main(int argc, char **argv) {
    printf("Trying to open server socket...\n");
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    printf("Success! Resource FD = %d\n", fd);
}
