#include <io.h>

// this is just port IO to connect to other parts of the motherboard and physical devices which are attached
// which communicate via IO. Ideally, most drivers will use MMIO, but some such as serial still use port IO.
// It's a bit slower than MMIO but required for many devices.

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %w1, %b0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}
