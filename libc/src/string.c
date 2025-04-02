#include <string.h>
#include <stdint.h>

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

void *memcpy(void *dest, const void *src, size_t n) {
    asm volatile(
        "rep movsb"
        : "=D"(dest), "=S"(src), "=c"(n)
        : "D"(dest), "S"(src), "c"(n)
        : "memory"
    );
    return dest;
}


void *memmove(void *dest, const void *src, size_t n) {
    if (dest == src) {
        return dest;
    } else if ((uintptr_t) dest < (uintptr_t) src) {
        return memcpy(dest, src, n);
    } else if ((uintptr_t) dest > (uintptr_t) src) {
        // copy in reverse
        asm volatile(
            "std\n"
            "rep movsb\n"
            "cld\n"
            : "=D"(dest), "=S"(src), "=c"(n)
            : "D"(dest + n - 1), "S"(src + n - 1), "c"(n)
            : "memory"
        );
        return dest;
    } else {
        return NULL; // unreachable
    }
}
