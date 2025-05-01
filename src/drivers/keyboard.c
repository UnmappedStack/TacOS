#include <kernel.h>
#include <io.h>
#include <pit.h>
#include <cpu.h>
#include <framebuffer.h>
#include <printf.h>
#include <fs/tempfs.h>

#define PS2_DATA_REGISTER    0x60
#define PS2_STATUS_REGISTER  0x64
#define PS2_COMMAND_REGISTER 0x64

uint8_t scancode_event_queue[30]; // if it gets to 30 we shift everything back
uint8_t nkbevents = 0;

char character_table[] = {
    0,    0,    '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
    '-',  '=',  0,    0x09, 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    'o',  'p',  '[',  ']',  0,    0,    'a',  's',  'd',  'f',  'g',  'h',
    'j',  'k',  'l',  ';',  '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',  0,    ' ',  0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x1B, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0x0E, 0x1C, 0,    0,    0,
    0,    0,    0,    0,    0,    '/',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0,
    0,    0,    0,    0,    0,    0,    0,    0x2C
};

char shifted_character_table[] = {
    0,    0,    '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
    '_',  '+',  0,    0x09, 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    'O',  'P',  '{',  '}',  0,    0,    'A',  'S',  'D',  'F',  'G',  'H',
    'J',  'K',  'L',  ':',  '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',  0,    ' ',  0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0x1B, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0x0E, 0x1C, 0,    0,    0,
    0,    0,    0,    0,    0,    '?',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0,
    0,    0,    0,    0,    0,    0,    0,    0x2C,
};

typedef struct {
    uint64_t input_len;
    bool     currently_reading;
    bool     shifted;
    bool     caps;
    char     *current_buffer;
    uint64_t buffer_size;
} KeyboardData;

KeyboardData current_input_data = {0};

void draw_cursor(void) {
    write_framebuffer_char('_');
    kernel.tty.loc_x -= 8;
}

void add_kb_event_to_queue(uint8_t scancode) {
    (void) scancode;
}

__attribute__((interrupt))
void keyboard_isr(void*) {
    if (!(inb(PS2_STATUS_REGISTER) & 0x01)) goto ret;
    uint8_t scancode = inb(PS2_DATA_REGISTER);
    if (!current_input_data.currently_reading) {
        add_kb_event_to_queue(scancode);
        goto ret;
    }
    // special cases
    // it's a release, not a press, OR an unprintable key
    if (scancode & 0x80 || scancode == 0x5B || scancode == 1) {
        if (scancode == 0xAA || scancode == 0xB6) // shift key is released
            current_input_data.shifted = false;
        goto ret;
    }
    if (scancode == 0x0E && current_input_data.input_len > 0) { // backspace
        // cover it up
        write_framebuffer_char(' ');
        kernel.tty.loc_x -= 16;
        write_framebuffer_char(' ');
        kernel.tty.loc_x -= 8;
        draw_cursor();
        // remove from buffer
        current_input_data.input_len--;
        current_input_data.current_buffer[current_input_data.input_len] = 0;
        goto ret;
    }
    if (scancode == 0x3A) { // capslock key
        current_input_data.caps = !current_input_data.caps;
        goto ret;
    }
    if (scancode == 0x2A || scancode == 0x36) { // one of the shift keys
        current_input_data.shifted = true;
        goto ret;
    }
    if (scancode == 0x1C || current_input_data.buffer_size == current_input_data.input_len) { // enter
        current_input_data.current_buffer[current_input_data.input_len] = 0;
        current_input_data.currently_reading = false;
        // remove the cursor
        printf(" ");
        kernel.tty.loc_x -= 8;
        goto ret;
    }
    char ch;
    if (current_input_data.shifted && !current_input_data.caps) {
        ch = shifted_character_table[scancode];
    } else if (current_input_data.shifted && current_input_data.caps) {
        if (character_table[scancode] >= 'a' && character_table[scancode] <= 'z')
            ch = character_table[scancode];
        else
            ch = shifted_character_table[scancode];
    } else if (!current_input_data.shifted && current_input_data.caps) {
        if (character_table[scancode] >= 'a' && character_table[scancode] <= 'z')
            ch = shifted_character_table[scancode];
        else
            ch = character_table[scancode];
    } else {
        ch = character_table[scancode];
    }
    write_framebuffer_char(ch);
    current_input_data.current_buffer[current_input_data.input_len] = ch;
    current_input_data.input_len++;
    draw_cursor();
ret:
    end_of_interrupt();
    return;
}


int kb_open(void *f) {
    (void) f;
    return 0;
}

int kb_close(void *f) {
    (void) f;
    return 0;
}

int kb_write(void *f, char *buf, size_t len, size_t off) {
    (void) f, (void) buf, (void) len, (void) off;
    return -1;
}

int kb_read(void *f, char *buf, size_t len, size_t off) {
    (void) f, (void) off;
    draw_cursor();
    current_input_data.currently_reading = true;
    current_input_data.current_buffer    = buf;
    current_input_data.buffer_size       = len - 1;
    if (inb(PS2_STATUS_REGISTER) & 0x01) inb(PS2_DATA_REGISTER);
    unlock_lapic_timer(); // Just in case it's disabled somewhere by mistake or smth
    while (current_input_data.currently_reading) IO_WAIT();
    current_input_data.current_buffer = 0;
    current_input_data.input_len      = 0;
    // clear the cursor
    kernel.tty.loc_x += 8;
    write_framebuffer_char(' ');
    kernel.tty.loc_x -= 8;
    return 0;
}

void init_keyboard() {
    DeviceOps kbdev_ops = (DeviceOps) {
        .read  = &kb_read,
        .write = &kb_write,
        .open  = &kb_open,
        .close = &kb_close,
        .is_term = false,
    };
    mkdevice("/dev/stdin0", kbdev_ops);
    set_IDT_entry(33, (void*) keyboard_isr, 0x8E, kernel.IDT);
    map_ioapic(33, 1, 0, POLARITY_HIGH, TRIGGER_EDGE);
}
