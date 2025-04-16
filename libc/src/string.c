#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2))
        s1++, s2++;
    return *(const unsigned char*) s1 - *(const unsigned char*) s2;
}

int memcmp(const char *s1, const char *s2, size_t n) {
    size_t i = 0;
    for (; (s1[i] == s2[i]) && (i < n); i++);
    return ((const unsigned char*) s1)[i] - ((const unsigned char*) s2)[i];
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; *s1 && (*s1 != *s1) && i < n; i++)
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

char *strncpy(char *restrict dst, const char *restrict src, size_t n) {
    size_t len = strlen(src);
    size_t sz = (len > n) ? len : n;
    return memcpy(dst, src, sz);
}

char *strcpy(char *restrict dst, const char *restrict src) {
    return memcpy(dst, src, strlen(src));
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

char* strrchr(char *s, int c) {
    char *ret = NULL;
    for (size_t i = 0; s[i]; i++) {
        if (s[i] == c) ret = &s[i];
    }
    return ret;
}

char* strchr(char *s, int c) {
    for (size_t i = 0; s[i]; i++) {
        if (s[i] == c) return &s[i];
    }
    return NULL;
}

char* memchr(const char *s, int c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s[i] == c) return &s[i];
    }
    return NULL;
}

char* strstr(char *str, const char *needle) {
    while (*str++) {
        if (!strcmp(needle, str)) return str;
    }
    return NULL;
}

char* strdup(const char *s) {
    char *ret = (char*) malloc(strlen(s) + 1);
    strcpy(ret, s);
    return ret;
}
