#include <kprintf.h>
#include <string.h>
#include <serial.h>
#include <stdarg.h>

// this has its own function despite being just a wrapper of write_serial because
// in the future it will be generic to both tty logging and serial.
void print_string(const char *s) {
    write_serial(s);
}

// this is a pretty simple kprintf implementation. it's similar to printf except not posix, so it
// doesn't have support for many things that posix printf supports such as floats. Why? Well, you just don't
// really need that stuff in the kernel, it's more useful in userspace.
//
// It supports %c, %s, %x pretty much the same as on posix, except %u is a bit different, as it formats 64 bit unsigned integers instead
// of unsigned smaller data sizes. It then just writes it to serial output.
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            write_serial_char(*fmt);
            continue;
        }
        char buf[64] = {0};
        switch (*(++fmt)) {
            case '%':
                write_serial_char('%');
                break;
            case 'c':
                write_serial_char(va_arg(args, int));
                break;
            case 's':
                write_serial(va_arg(args, char*));
                break;
            case 'u':
                uint64_to_string(va_arg(args, uint64_t), buf);
                write_serial(buf);
                break;
            case 'x':
                uint64_to_hex_string(va_arg(args, uint64_t), buf);
                write_serial("0x");
                write_serial(buf);
                break;
            default: break;
        }
    }
    va_end(args);
}
