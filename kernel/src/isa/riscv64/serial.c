#include <serial.h>

#define UART_MMIO_ADDR 0x10000000

void write_serial_char(char ch) {
    *((unsigned char*)UART_MMIO_ADDR) = ch;
}

void write_serial(const char *s) {
    for (; *s; s++) write_serial_char(*s);
}

void serial_init(void) {
    write_serial("Serial init OK\n");
}
