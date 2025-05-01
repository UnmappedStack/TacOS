#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("Just a little testing utility for the keyboard driver's non-IO-blocking queue functionality.\n");
        exit(-1);
    }
    for (;;);
}
