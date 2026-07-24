#include <string.h>

// using rep movsb is much faster than looping over bytes when copying large amounts of
// data on x86. However, this should be looping over data when it's a smaller amount of data
// as it is more efficient for smaller pools of memory to loop. (TODO)
void *memcpy(void *dest, const void *src, size_t n) {
    __asm__ volatile("rep movsb"
                     : "=D"(dest), "=S"(src), "=c"(n)
                     : "D"(dest), "S"(src), "c"(n)
                     : "memory");
    return dest;
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

