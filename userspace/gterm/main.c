#include <stdlib.h>
#include <string.h>
#include <pty.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("GTerm: A simple graphical terminal emulator.\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }
    int e;
    int master, slave;
    e=openpty(&master, &slave, NULL, NULL, NULL);
    if (e < 0) {
        fprintf(stderr, "Failed to open pty\n");
        return -1;
    }
    printf("master resource: %d\n"
           "slave resource: %d\n",
           master, slave);
    char *msg = "Hello world from slave->master!";
    e=write(slave, msg, strlen(msg)+1);
    if (e < 1) {
        fprintf(stderr, "Failed to write to slave\n");
        return -1;
    }
    printf("Successfully write message to slave!\n");
    char buf[30];
    e=read(master, buf, 30);
    if (e < 1) {
        fprintf(stderr, "Failed to read from master\n");
        return -1;
    }
    printf("Successfully read back from master! Message: \"%s\"\n", buf);
    return 0;
}
