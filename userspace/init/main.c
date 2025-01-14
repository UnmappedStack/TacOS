#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    int f = open("/dev/tty0", 0, 0);
    char *msg = "Hello world from a userspace application, loaded from an ELF file!\n";
    write(f, msg, strlen(msg) - 1);
    return 0;
}
