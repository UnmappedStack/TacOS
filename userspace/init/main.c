#include <string.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // TODO: Open stdin and stderr, tty0 (stdout) becomes fd 1 instead of 0
    int f = open("/dev/tty0", 0, 0);
    puts("Hello world from a userspace application loaded from an ELF file, writing to a stdout device!\n");
    printf("Hello, world! The number is %d and %d\n", atoi("69"), atoi("\t\n  -420abc"));
    return 0;
}
