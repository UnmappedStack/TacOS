#include <serial.h>

// To start serial, some config registers must be set. in my opinion it is not too important
// to know what they are for, it is pretty stock-standard setup, so I did not comment them.
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

    /* TODO: Uncomment this but make it only be done when a serial device is detected
    // the COM1 port with an offset of 0 is the recieve/reading buffer.
    // when it gives the value 0xAE, it is ready to be used and not faulty, so we wait for such an event.
    if(inb(COM1) != 0xAE)
        for (;;);
    */

    // finally we go into normal operation mode
    outb(COM1 + 4, 0x0F);
}

void write_serial_char(char ch) {
    // offset 5 is the line status register. Here, I read 0x20, or bit 5, which
    // is set when the transmission buffer is empty. Poll for it to stop being clear
    // so that we can send data...
    while ((inb(COM1 + 5) & 0x20) == 0);
    outb(COM1, ch); // ...then just output the character to the transmit buffer.
}

// simply loops through characters in a const char *s and calls write_serial_char() on each
void write_serial(const char *s) {
    for (; *s; s++) write_serial_char(*s);
}
