#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("GuacUtils clear: A simple shell utility for clearing the TTY.\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }
    printf("\x1b[2J");
    fflush(stdout);
    return 0;
}
