#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("GuacUtils echo: A simple shell utility for repeating the arguments to standard output.\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}
