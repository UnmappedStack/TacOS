#include <stdio.h>
#include <string.h>
#include <mman.h>
#include <unistd.h>
#include <stdlib.h>

void cat_file(char **argv, int f) {
    if (f < 0) {
        printf("cat: Failed to open file: %s\n", argv[1]);
        exit(-1);
    }
    off_t sz = lseek(f, 0, SEEK_END);
    lseek(f, 0, SEEK_SET);
    sz = (sz) ? sz : 256;
    char *buf = (char*) malloc(sz);
    read(f, buf, sz);
    buf[sz] = 0;
    if (sz) printf("\n");
    puts(buf);
}

int main(int argc, char **argv) {
    if (argc > 2) {
        printf("cat only takes one argument, being the file to read from.\n");
        return -1;
    }
    if (argv[1][0] == '-') {
        printf("GuacUtils cat: A simple shell utility for writing the contents of a file to standard output.\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }
    int f = (argc > 1) ? open(argv[1], 0, 0) : fileno(stdin);
    for (int i = 0; i < 1 || argc <= 1; i++)
        cat_file(argv, f);
    if (f != fileno(stdin)) close(f);
    return 0;
}
