#include <string.h>

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}
void *memset (void *dest, int x, size_t n) {
    asm volatile(
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

void *memcpy (void *dest, const void *src, size_t n) {
    asm volatile(
        "rep movsb"
        : "=D"(dest), "=S"(src), "=c"(n)
        : "D"(dest), "S"(src), "c"(n)
        : "memory"
    );
    return dest;
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
