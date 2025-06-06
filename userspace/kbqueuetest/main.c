#include <stdio.h>
#include <TacOS.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("Just a little testing utility for the keyboard driver's non-IO-blocking queue functionality.\n"
               "Copyright 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0.\n");
        exit(-1);
    }
    int f = open("/dev/kb0", 0, 0);
    printf("Waiting for a key...\n");
    for (;;) {
        Key key;
        if (read(f, &key, 1) < 0) {
            printf("Failed to get key\n");
            close(f);
            return -1;
        } else if (key & 0x80) {
            printf("Key released: %x\n", key & 0x7f);
        } else if (key != KeyNoPress) {
            printf("Key pressed! Val: %x\n", key);
        }
    }
    close(f);
}
