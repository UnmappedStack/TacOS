#include <string.h>
#include <printf.h>

size_t oct2bin(char *str, int size) {
    int n = 0;
    char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

void *memset(void *dest, int x, size_t n) {
    __asm__ volatile(
        "rep stosb"
        : "=D"(dest), "=c"(n)
        : "D"(dest), "a"(x), "c"(n)
        : "memory"
    );
    return dest;
}

char *strcpy(char *dest, const char *src) {
    size_t src_len = strlen(src);
    for (size_t i = 0; i < src_len; i++) {
        dest[i] = src[i];
    }
    dest[src_len] = 0;
    return dest;
}

// NON POSIX!!!!
// If it doesn't match, it always returns 1.
int strcmp(char *str1, char *str2) {
    while (*str1) {
        if (!*str2) return 1;
        if (*str1 != *str2) return 1;
        str1++;
        str2++;
    }
    return (*str2);
}

// NOTE: also non posix, see comment for strcmp above
int memcmp(char *str1, char *str2, size_t bytes) {
    for (size_t i = 0; i < bytes; i++) {
        if (str1[i] != str2[i]) return 1;
    }
    return 0;
}

void *memcpy(void *dest, const void *src, size_t n) {
    __asm__ volatile(
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
        __asm__ volatile(
            "std\n"
            "rep movsb\n"
            "cld\n"
            : "=D"(dest), "=S"(src), "=c"(n)
            : "D"((uintptr_t) dest + n - 1), "S"((uintptr_t) src + n - 1), "c"(n)
            : "memory"
        );
        return dest;
    } else {
        // unreachable
        return NULL;
    }
}

void reverse(char str[], int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void uint64_to_binary_string(uint64_t num, char *buf) {
    char buffer[65];
    int idx = 0;
    if (num == 0) {
        buffer[idx++] = '0';
    } else {
        while (num > 0) {
            buffer[idx++] = (num & 1) ? '1' : '0';
            num >>= 1;
        }
    }
    buffer[idx] = 0;
    reverse(buffer, idx);
    for (int i = 0; i <= idx; i++)
        buf[i] = buffer[i];
}

void uint64_to_hex_string_padded(uint64_t num, char *str) {
    char buffer[17];
    int index = 0;
    if (num == 0) {
        buffer[index++] = '0';
    } else {
        while (num > 0) {
            uint8_t digit = num & 0xF;
            if (digit < 10) {
                buffer[index++] = '0' + digit;
            } else {
                buffer[index++] = 'A' + (digit - 10);
            }
            num >>= 4;
        }
    }

    while (index < 16)
        buffer[index++] = '0';

    buffer[index] = '\0';
    reverse(buffer, index);
    memcpy(str, buffer, 17);
}

void uint64_to_hex_string(uint64_t num, char *str) {
    char buffer[17];
    int index = 0;
    if (num == 0) {
        buffer[index++] = '0';
    } else {
        while (num > 0) {
            uint8_t digit = num & 0xF;
            if (digit < 10) {
                buffer[index++] = '0' + digit;
            } else {
                buffer[index++] = 'A' + (digit - 10);
            }
            num >>= 4;
        }
    }

    buffer[index] = '\0';
    reverse(buffer, index);
    memcpy(str, buffer, 17);
}

int get_num_length(uint64_t num) {
    int length = 0;
    do {
        length++;
        num /= 10;
    } while (num > 0);
    return length;
}

void uint64_to_string(uint64_t num, char* str) {
    int length = get_num_length(num);
    str[length] = '\0';
    int index = length - 1;
    do {
        str[index--] = '0' + (num % 10);
        num /= 10;
    } while (num > 0);
}
