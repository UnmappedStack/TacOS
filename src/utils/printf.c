#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <printf.h>
#include <serial.h>

void write_text(char *text) {
    write_serial(text);
}

void write_character(char ch) {
    write_serial_char(ch);
}

void printf_template(char* format, va_list args) {
    size_t i   = 0;
    size_t len = strlen(format);
    while (i < len) {
        if (format[i] == '%') {
            i++;
            char buffer[10];
            if (format[i] == 'd' || format[i] == 'i') {
                uint64_to_string(va_arg(args, uint64_t), buffer);
                buffer[9] = 0;
                write_text(buffer);
            } else if (format[i] == 'c') {
                char character = va_arg(args, int);
                write_character(character);
            } else if (format[i] == 'x') {
                char bufferx[20];
                uint64_to_hex_string(va_arg(args, uint64_t), bufferx);
                bufferx[19] = 0;
                write_text(bufferx);
            } else if (format[i] == 'p') {
                char bufferx[20];
                uint64_to_hex_string_padded(va_arg(args, uint64_t), bufferx);
                bufferx[19] = 0;
                write_text(bufferx);
            } else if (format[i] == 'b') {
                char bufferb[65];
                uint64_to_binary_string(va_arg(args, uint64_t), bufferb);
                bufferb[64] = 0;
                write_text(bufferb);
            } else if (format[i] == 's') {
                write_text(va_arg(args, char*));
            }
        } else {
            write_character(format[i]);
        }
        i++;
    }
}

void printf(char* format, ...) {
    va_list args;
    va_start(args, format);
    printf_template(format, args);
    va_end(args);
}
