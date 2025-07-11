#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

char *strerror(int errnum) {
    return "Unknown errno";
}

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

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    while (n--) {
        if (*p1 != *p2)
            return (int)(*p1 - *p2);
        p1++;
        p2++;
    }
    return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; *s1 && (*s1 != *s2) && i < n; i++)
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

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < n; i++)
        dest[i] = '\0';
   return dest;
}

char *strcpy(char *restrict dst, const char *restrict src) {
    return memcpy(dst, src, strlen(src) + 1);
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
        if (s[i] == (char) c) return &s[i];
    }
    return NULL;
}

char* strstr(const char *str, const char *needle) {
    while (*str++) {
        if (!strcmp(needle, str)) return str;
    }
    return NULL;
}

char* strdup(const char *s) {
    size_t len = strlen(s);
    char *ret = (char*) malloc(len + 1);
    memcpy(ret, s, len);
    ret[len] = 0;
    return ret;
}

char* strndup(char *s, size_t n) {
    size_t slen = strlen(s);
    size_t len = (slen > n) ? n : slen;
    int *t = malloc(1); // I'm giving in to the UB. Fuck this.
                        // Don't worry, this'll be gone when I switch to newlib.
    char *ret = (char*) malloc(len + 1);
    free(t);
    memcpy(ret, s, len);
    ret[len] = 0;
    return ret;
}

char *strpbrk(const char *s, const char *accept) {
    size_t naccept = strlen(accept);
    for (; *s; s++) {
        for (size_t i = 0; i < naccept; i++) {
            if (accept[i] == *s) return s;
        }
    }
    return NULL;
}

size_t strspn(const char *s, const char *accept) {
    size_t naccept = strlen(accept);
    size_t ret = 0;
    char *cpy = s;
    for (; *s; s++) {
        bool found = false;
        for (size_t i = 0; i < naccept; i++) {
            if (accept[i] == *s) {
                found = true;
                continue;
            }
        }
        if (!found) return ret;
        ret++;
    }
    return strlen(cpy);
}

// TODO this is a stub cos it's meant to properly convert locales and all that stuff
int strcoll(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}
