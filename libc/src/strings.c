#include <string.h>

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 != *s2 || tolower(*s1) != tolower(*s2)))
        s1++, s2++;
    return *(const unsigned char*) s1 - *(const unsigned char*) s2;
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    size_t i = 0;
    while (i < n && *s1 && (*s1 != *s2 || tolower(*s1) != tolower(*s2)))
        s1++, s2++;
    return *(const unsigned char*) s1 - *(const unsigned char*) s2;
}
