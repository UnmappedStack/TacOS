#include <string.h>
#include <mm.h>

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

