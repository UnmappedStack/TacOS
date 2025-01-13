#include <string.h>

int open(char *fname) {
    int ret;
    asm volatile (
        "int $0x80\n" : "=a" (ret) : "a" (2), "D" (fname), "d" (strlen(fname))
    );
    return ret;
}

void write(int fd, char *buf, int len) {
    asm volatile (
        "int $0x80" : : "a" (1), "D" (fd), "S" (buf), "d" (len)
    );
}

int main() {
    int f = open("/dev/tty0");
    char *msg = "Hello world from a userspace application, loaded from an ELF file!\n";
    write(f, msg, strlen(msg));
    return 0;
}
