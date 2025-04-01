#include <string.h>

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 != *s1))
        s1++, s2++;
    return *(const unsigned char*) s1 - *(const unsigned char*) s2;
}

void *memset(void *dest, int x, size_t n) {
    asm volatile(
        "rep stosb"
        : "=D"(dest), "=c"(n)
        : "D"(dest), "a"(x), "c"(n)
        : "memory"
    );
    return dest;
}
