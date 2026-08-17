#include <serial.h>
#include <isa/cpu.h>

void write_serial_char(char c) {
    sbi_write_char(c);
}

void write_serial(const char *s) {
    for (; *s; s++) write_serial_char(*s);
}

void serial_init(void) {
    write_serial("Serial init OK\n");
}
