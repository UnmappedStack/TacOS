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
