#include <serial.h>

void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
    outb(COM1 + 4, 0x1E);
    outb(COM1 + 0, 0xAE);
    if(inb(COM1) != 0xAE)
        for (;;);
    outb(COM1 + 4, 0x0F);
}

void write_serial_char(char ch) {
    while ((inb(COM1+5)&0x20) == 0);
    outb(COM1, ch);
}

void write_serial(const char *s) {
    for (; *s; s++) write_serial_char(*s);
}
