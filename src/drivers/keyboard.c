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

// this should also have a copy in the libc (see /libc/include/TacOS.h)
typedef enum {
    CharA, CharB, CharC, CharD, CharE, CharF, CharG, CharH, CharI, CharJ, CharK,
    CharL, CharM, CharN, CharO, CharP, CharQ, CharR, CharS, CharT, CharU, CharV,
    CharW, CharX, CharY, CharZ,
    Dec0, Dec1, Dec2, Dec3, Dec4, Dec5, Dec6, Dec7, Dec8, Dec9,
    KeyEnter, KeyShift, KeySpace, KeyForwardSlash, KeyBackSlash, KeyComma,
    KeySingleQuote, KeySemiColon, KeyLeftSquareBracket, KeyRightSquareBracket,
    KeyEquals, KeyMinus, KeyBackTick, KeyAlt, KeySuper, KeyTab,
    KeyCapsLock, KeyEscape, KeyBackspace, KeyLeftArrow, KeyRightArrow,
    KeyUpArrow, KeyDownArrow, KeyControl, KeyUnknown, KeyNoPress,
} Key;

Key scancode_event_queue[30]; // if it gets to 30 we shift everything back
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

#define KB_MAX_LEN 200
typedef struct {
    uint64_t input_len;
    bool     currently_reading;
    bool     shifted;
    bool     caps;
    char     current_buffer[KB_MAX_LEN];
    uint64_t buffer_size;
} KeyboardData;

KeyboardData current_input_data = {0};

void draw_cursor(bool override) {
    // kinda hacky but until there's canonical mode, we just return immediately
    // if only reading one key
    if (!current_input_data.buffer_size && !override) return;
    write_framebuffer_char_nocover('_');
    kernel.tty.loc_x -= 8;
}

Key scancode_to_key(uint8_t scancode) {
    scancode &= 0x7f;
    char as_char = character_table[scancode];
    if      (as_char >= 'a' && as_char <= 'z') return as_char - 'a';
    else if (as_char >= '0' && as_char <= '9')
        return ('z' - 'a' + 1) + as_char - '0';
    int release_flag = (scancode & 0x80) ? 0x80 : 0;
    switch (scancode) {
    case 0x5a:
    case 0x1c: return release_flag | KeyEnter;
    case 0x2a:
    case 0x36: return release_flag | KeyShift;
    case 0x39: return release_flag | KeySpace;
    case 0x35: return release_flag | KeyForwardSlash;
    case 0x2b: return release_flag | KeyBackSlash;
    case 0x33: return release_flag | KeyComma;
    case 0x52: return release_flag | KeySingleQuote;
    case 0x27: return release_flag | KeySemiColon;
    case 0x54: return release_flag | KeyLeftSquareBracket;
    case 0x1b: return release_flag | KeyRightSquareBracket;
    case 0x0d: return release_flag | KeyEquals;
    case 0x0c: return release_flag | KeyMinus;
    case 0x29: return release_flag | KeyBackTick;
    case 0x38: return release_flag | KeyAlt;
    case 0x5b: return release_flag | KeySuper;
    case 0x0f: return release_flag | KeyTab;
    case 0x3a: return release_flag | KeyCapsLock;
    case 0x01: return release_flag | KeyEscape;
    case 0x0e: return release_flag | KeyBackspace;
    case 0x4b: return release_flag | KeyLeftArrow;
    case 0x4d: return release_flag | KeyRightArrow;
    case 0x1d:
    case 0xe0: return release_flag | KeyControl;
    case 0x48: return release_flag | KeyUpArrow;
    case 0x50: return release_flag | KeyDownArrow;
    default:   return release_flag | KeyUnknown;
    }
}

void add_kb_event_to_queue(uint8_t scancode) {
    Key key = scancode_to_key(scancode);
    if (scancode & 0x80) key |= 0x80;
    scancode_event_queue[nkbevents++] = key;
    if (nkbevents > 29) {
        // shift everything back
        memmove(scancode_event_queue, &scancode_event_queue[1], 29);
        nkbevents--;
    }
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
    // if it's the first character and the backspace key is pressed
    if (!current_input_data.input_len && scancode == 0x0E && current_input_data.buffer_size < 1) {
        // don't draw it but add 127 to the buffer
        current_input_data.current_buffer[current_input_data.input_len++] = 127;
        goto finishup;
    }
    // if it's an arrow key, return the appropriate code
    if (scancode == 0x4b || scancode == 0x4d || scancode == 0x48 || scancode == 0x50) {
        int ret;
        switch (scancode) {
        case 0x4b:
            ret = 126;
            break;
        case 0x4d:
            ret = 125;
            break;
        case 0x48:
            ret = 123;
            break;
        case 0x50:
            ret = 124;
            break;
        }
        current_input_data.current_buffer[current_input_data.input_len++] = ret;
        draw_cursor(false);
        goto finishup;
    }
    // it's a release, not a press, OR an unprintable key
    if (scancode & 0x80 || scancode == 1) {
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
        draw_cursor(false);
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
    if (scancode == 0x1C) { // enter
        current_input_data.current_buffer[current_input_data.input_len] = '\n';
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
    if (current_input_data.buffer_size)
        write_framebuffer_char(ch);
    current_input_data.current_buffer[current_input_data.input_len++] = ch;
    // if it's too long, finish up
finishup:
    if (current_input_data.buffer_size <= current_input_data.input_len) {
        current_input_data.current_buffer[current_input_data.input_len] = 0;
        current_input_data.currently_reading = false;
    } else {
        draw_cursor(false);
    }
ret:
    end_of_interrupt();
}


int stdin_open(void *f) {
    (void) f;
    return 0;
}

int stdin_close(void *f) {
    (void) f;
    return 0;
}

int stdin_write(void *f, char *buf, size_t len, size_t off) {
    (void) f, (void) buf, (void) len, (void) off;
    return -1;
}

// TODO: off param needs to be taken into account
int stdin_read(void *f, char *buf, size_t len, size_t off) {
    (void) f, (void) off;
    current_input_data.currently_reading = true;
    current_input_data.buffer_size       = len - 1;
    if (inb(PS2_STATUS_REGISTER) & 0x01) inb(PS2_DATA_REGISTER);
    draw_cursor(false);
    while (current_input_data.currently_reading) IO_WAIT();
    memcpy(buf, current_input_data.current_buffer, current_input_data.input_len + 1);
    size_t ret = current_input_data.input_len;
    current_input_data.input_len = 0;
    // clear the cursor
    if (len > 1 && buf[0] != 127) {
        kernel.tty.loc_x += 8;
        write_framebuffer_char(' ');
        kernel.tty.loc_x -= 8;
    }
    return ret + 1;
}

int kb_read(void *f, Key *buf, size_t len, size_t off) {
    (void) f, (void) off;
    if (len < 1) return 0;
    if (!nkbevents) {
        *buf = KeyNoPress;
        return 0;
    }
    *buf = scancode_event_queue[--nkbevents];
    return 0;
}

void init_keyboard() {
    DeviceOps stdindev_ops = (DeviceOps) {
        .read  = &stdin_read,
        .write = &stdin_write,
        .open  = &stdin_open,
        .close = &stdin_close,
        .is_term = false,
    };
    DeviceOps kbdev_ops = (DeviceOps) {
        .read  = (void*) &kb_read,
        .write = &stdin_write,
        .open  = &stdin_open,
        .close = &stdin_close,
        .is_term = false,
    };
    mkdevice("/dev/stdin0", stdindev_ops);
    mkdevice("/dev/kb0", kbdev_ops);
    set_IDT_entry(33, (void*) keyboard_isr, 0x8E, kernel.IDT);
    map_ioapic(33, 1, 0, POLARITY_HIGH, TRIGGER_EDGE);
}
