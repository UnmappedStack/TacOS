#include <string.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>

void puts(char *str) {
    write(0, str, strlen(str));
}

int main() {
    // TODO: Open stdin and stderr, tty0 (stdout) becomes fd 1 instead of 0
    int f = open("/dev/tty0", 0, 0);
    puts("Hello world from a userspace application loaded from an ELF file, writing to a stdout device!\n");
    char *buf = (char*) malloc(100);
    sprintf(buf, "The number is %d\n", 5);
    puts(buf);
    free(buf);
}
