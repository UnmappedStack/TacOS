#include <string.h>
#include <mm.h>
#include <isa/cpu.h>

void *memcpy(void *dest, const void *src, size_t n) {
    // rep movsb is faster for large buffers if its x86...
#if defined(__x86_64__)
    if (n >= PAGE_BYTES * 2) {
        __asm__ volatile("rep movsb"
                         : "=D"(dest), "=S"(src), "=c"(n)
                         : "D"(dest), "S"(src), "c"(n)
                         : "memory");
        return dest;
    }
#endif
    // ...but it is actually faster to manually loop if its a relatively small memory buffer
    for (size_t i = 0; i < n; i++) {
        ((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
    }
    return dest;
}

void *memset(void *dest, int ch, size_t n) {
#if defined(__x86_64__)
    if (n >= PAGE_BYTES * 2) {
        __asm__ volatile("rep stosb" : "+D"(dest), "+c"(n) : "a"(ch) : "memory");
        return dest;
    }
#endif
    for (size_t i = 0; i < n; i++)
        ((uint8_t*)dest)[i] = ch;
    return dest;
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

int strcmp(const char *s1, const char *s2) {
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    if (len1 != len2) return 1;
    return memcmp(s1, s2, len1);
}

// Gets the number of digits of a base 10 number
int get_num_length(uint64_t num) {
    int length = 0;
    do {
        length++;
        num /= 10;
    } while (num > 0);
    return length;
}

void uint64_to_string(uint64_t num, char *str) {
    int length = get_num_length(num);
    str[length+1] = '\0';
    int index = length - 1;
    do {
        str[index--] = '0' + (num % 10);
        num /= 10;
    } while (num > 0);
}

// reverses the digits of a string
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
    while (index < 16) buffer[index++] = '0';
    buffer[index] = '\0';
    reverse(buffer, index);
    memcpy(str, buffer, 17);
}

uint64_t str_to_u64(const char *str) {
    uint64_t result = 0;
    while (*str) {
        if (*str < '0' || *str > '9')
            break;
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

uint64_t strlen(const char *str) {
    uint64_t ret = 0;
    for (; *str; str++) ret++;
    return ret;
}

// idk if its stupid to have this in string.c instead of a generic utils.c but wtv
int count_leading_zeroes(uint64_t x) {
    if (!x) kpanic("count_leading_zeroes given 0");
#ifdef __x86_64__
    return __builtin_ctzll(x);
#endif
    // this is much slower than __builtin_ctzll
    int ret = 0;
    for (size_t i = 0; i < 64; i++) {
        if (x & (1ULL << i)) return ret;
        ret++;
    }
    return -1; //theoretically unreachable
}
