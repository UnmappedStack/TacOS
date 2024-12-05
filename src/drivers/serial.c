#include <string.h>
#include <serial.h>
#include <io.h>

#define COM1 0x3f8

void init_serial() {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
    outb(COM1 + 4, 0x1E);
    outb(COM1 + 0, 0xAE);
    if(inb(COM1 + 0) != 0xAE) {
        return;
    }
    outb(COM1 + 4, 0x0F);
}

static int is_transmit_empty() {
    return inb(COM1 + 5) & 0x20;
}

void write_serial_char(char ch) {
    while (!is_transmit_empty());
    outb(COM1, ch);
}

void write_serial(const char *str) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++)
        write_serial_char(str[i]);
}
