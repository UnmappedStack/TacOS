#include <string.h>
#include <stdio.h>
#include <ctype.h>

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && (tolower(*s1) == tolower(*s2)))
        s1++, s2++;
    return tolower(*(const unsigned char*) s1) - tolower(*(const unsigned char*) s2);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    size_t i = 0;
    while (i < n && *s1 && (tolower(*s1) == tolower(*s2)))
        s1++, s2++, i++;
    if (i == n) return 0;
    int ret = tolower(*(const unsigned char*) s1) - tolower(*(const unsigned char*) s2);
    return ret;
}
