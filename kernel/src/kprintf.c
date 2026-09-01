#include <kprintf.h>
#include <tty.h>
#include <lock.h>
#include <string.h>
#include <serial.h>
#include <stdarg.h>

void print_char(char c) {
    write_serial_char(c);
    tty_write_char(c);
}

void print_string(const char *s) {
    write_serial(s);
    tty_write_text(s);
}

// this is a pretty simple kprintf implementation. it's similar to printf except not posix, so it
// doesn't have support for many things that posix printf supports such as floats. Why? Well, you just don't
// really need that stuff in the kernel, it's more useful in userspace.
//
// It supports %c, %s, %x pretty much the same as on posix, except %u is a bit different, as it formats 64 bit unsigned integers instead
// of unsigned smaller data sizes. It then just writes it to serial output.
Spinlock kprintf_lock = {0};
void kprintf(const char *fmt, ...) {
    spinlock_acquire(&kprintf_lock);
    va_list args;
    va_start(args, fmt);
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            print_char(*fmt);
            continue;
        }
        char buf[64] = {0};
        switch (*(++fmt)) {
            case '%':
                print_char('%');
                break;
            case 'c':
                print_char(va_arg(args, int));
                break;
            case 's':
                char *s = va_arg(args, char*);
                if (!s) {
                    print_string("(null)");
                    break;
                }
                print_string(s);
                break;
            case 'u':
                uint64_to_string(va_arg(args, uint64_t), buf);
                print_string(buf);
                break;
            case 'i':
                int64_t uval = va_arg(args, int64_t);
                uint64_t pos = (uval >= 0) ? uval : -uval;
                if (uval < 0) print_string("-");
                uint64_to_string(pos, buf);
                print_string(buf);
                break;
            case 'x':
                uint64_to_hex_string(va_arg(args, uint64_t), buf);
                print_string("0x");
                print_string(buf);
                break;
            default: break;
        }
    }
    spinlock_release(&kprintf_lock);
    va_end(args);
}
