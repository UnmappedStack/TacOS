#include <serial.h>
#include <printf.h>

void _start() {
    write_serial("Hello from serial! This is TacOS, my latest OS kernel.\n"
                 "Every OS I write is a little better. SpecOS was terrible. PotatOS was better but still kinda broken.\n"
                 "This one, I wanna make genuinely the best one I've ever done.\n");
    printf("Printf test - Number: %i, binary literal: %b, hex literal: %x, string: %s\n",
            69, 0b10011001, 0x42, "hi there");
    for (;;);
}
