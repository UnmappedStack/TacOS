#include <string.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>

char buf[100];

void puts(char *str) {
    write(0, str, strlen(str));
}

int main() {
    // TODO: Open stdin and stderr, tty0 (stdout) becomes fd 1 instead of 0
    int f = open("/dev/tty0", 0, 0);
    exit(f);
    puts("Hello world from a userspace application loaded from an ELF file, writing to a stdout device!\n");
    exit(0);
    exit((int) &sprintf);
}
