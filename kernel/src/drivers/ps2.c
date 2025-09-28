#include <cpu.h>
#include <ps2.h>
#include <framebuffer.h>
#include <fs/tempfs.h>
#include <io.h>
#include <kernel.h>
#include <pit.h>
#include <printf.h>

// this should also have a copy in the libc (see /libc/include/TacOS.h)
typedef enum {
    CharA,
    CharB,
    CharC,
    CharD,
    CharE,
    CharF,
    CharG,
    CharH,
    CharI,
    CharJ,
    CharK,
    CharL,
    CharM,
    CharN,
    CharO,
    CharP,
    CharQ,
    CharR,
    CharS,
    CharT,
    CharU,
    CharV,
    CharW,
    CharX,
    CharY,
    CharZ,
    Dec0,
    Dec1,
    Dec2,
    Dec3,
    Dec4,
    Dec5,
    Dec6,
    Dec7,
    Dec8,
    Dec9,
    KeyEnter,
    KeyShift,
    KeySpace,
    KeyForwardSlash,
    KeyBackSlash,
    KeyComma,
    KeySingleQuote,
    KeySemiColon,
    KeyLeftSquareBracket,
    KeyRightSquareBracket,
    KeyEquals,
    KeyMinus,
    KeyBackTick,
    KeyAlt,
    KeySuper,
    KeyTab,
    KeyCapsLock,
    KeyEscape,
    KeyBackspace,
    KeyLeftArrow,
    KeyRightArrow,
    KeyUpArrow,
    KeyDownArrow,
    KeyControl,
    KeyFullStop,
    KeyUnknown,
    KeyNoPress,
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
    0,    0,    0,    0,    0,    0,    0,    0x2C};

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
    bool currently_reading;
    bool shifted;
    bool caps;
    char current_buffer[KB_MAX_LEN];
    uint64_t buffer_size;
} KeyboardData;

KeyboardData current_input_data = {0};

void draw_cursor(bool override) {
    // kinda hacky but until there's canonical mode, we just return immediately
    // if only reading one key
    if (!current_input_data.buffer_size && !override)
        return;
    write_framebuffer_char_nocover('_');
    kernel.tty.loc_x -= 8;
}

Key scancode_to_key(uint8_t scancode) {
    scancode &= 0x7f;
    char as_char = character_table[scancode];
    if (as_char >= 'a' && as_char <= 'z')
        return as_char - 'a';
    else if (as_char >= '0' && as_char <= '9')
        return ('z' - 'a' + 1) + as_char - '0';
    int release_flag = (scancode & 0x80) ? 0x80 : 0;
    switch (scancode) {
    case 0x5a:
    case 0x1c:
        return release_flag | KeyEnter;
    case 0x2a:
    case 0x36:
        return release_flag | KeyShift;
    case 0x39:
        return release_flag | KeySpace;
    case 0x35:
        return release_flag | KeyForwardSlash;
    case 0x2b:
        return release_flag | KeyBackSlash;
    case 0x33:
        return release_flag | KeyComma;
    case 0x52:
        return release_flag | KeySingleQuote;
    case 0x27:
        return release_flag | KeySemiColon;
    case 0x54:
        return release_flag | KeyLeftSquareBracket;
    case 0x1b:
        return release_flag | KeyRightSquareBracket;
    case 0x0d:
        return release_flag | KeyEquals;
    case 0x0c:
        return release_flag | KeyMinus;
    case 0x29:
        return release_flag | KeyBackTick;
    case 0x38:
        return release_flag | KeyAlt;
    case 0x5b:
        return release_flag | KeySuper;
    case 0x0f:
        return release_flag | KeyTab;
    case 0x3a:
        return release_flag | KeyCapsLock;
    case 0x01:
        return release_flag | KeyEscape;
    case 0x0e:
        return release_flag | KeyBackspace;
    case 0x4b:
        return release_flag | KeyLeftArrow;
    case 0x4d:
        return release_flag | KeyRightArrow;
    case 0x1d:
    case 0xe0:
        return release_flag | KeyControl;
    case 0x48:
        return release_flag | KeyUpArrow;
    case 0x50:
        return release_flag | KeyDownArrow;
    case 0x34:
        return release_flag | KeyFullStop;
    default:
        return release_flag | KeyUnknown;
    }
}

void add_kb_event_to_queue(uint8_t scancode) {
    Key key = scancode_to_key(scancode);
    if (scancode & 0x80)
        key |= 0x80;
    scancode_event_queue[nkbevents++] = key;
    if (nkbevents > 29) {
        // shift everything back
        memmove(scancode_event_queue, &scancode_event_queue[1], 29);
        nkbevents--;
    }
}

__attribute__((interrupt)) void keyboard_isr(void *) {
    if (!(inb(PS2_STATUS_REGISTER) & 0x01))
        goto ret;
    uint8_t scancode = inb(PS2_DATA_REGISTER);
    if (!current_input_data.currently_reading) {
        add_kb_event_to_queue(scancode);
        goto ret;
    }
    // special cases
    // if it's the first character and the backspace key is pressed
    if (!current_input_data.input_len && scancode == 0x0E &&
        current_input_data.buffer_size < 1) {
        // don't draw it but add 127 to the buffer
        current_input_data.current_buffer[current_input_data.input_len++] = 127;
        goto finishup;
    }
    // if it's an arrow key, return the appropriate code
    if (scancode == 0x4b || scancode == 0x4d || scancode == 0x48 ||
        scancode == 0x50) {
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
        if (character_table[scancode] >= 'a' &&
            character_table[scancode] <= 'z')
            ch = character_table[scancode];
        else
            ch = shifted_character_table[scancode];
    } else if (!current_input_data.shifted && current_input_data.caps) {
        if (character_table[scancode] >= 'a' &&
            character_table[scancode] <= 'z')
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

int stdin_open(void **buf, void *f) {
    *buf = f;
    return 0;
}

int stdin_close(void *f) {
    (void)f;
    return 0;
}

int stdin_write(void *f, char *buf, size_t len, size_t off) {
    (void)f, (void)buf, (void)len, (void)off;
    return -1;
}

// TODO: off param needs to be taken into account
int stdin_read(void *f, char *buf, size_t len, size_t off) {
    (void)f, (void)off;
    current_input_data.currently_reading = true;
    current_input_data.buffer_size = len - 1;
    if (inb(PS2_STATUS_REGISTER) & 0x01)
        inb(PS2_DATA_REGISTER);
    draw_cursor(false);
    while (current_input_data.currently_reading) IO_WAIT();
    memcpy(buf, current_input_data.current_buffer,
           current_input_data.input_len + 1);
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
    (void)f, (void)off;
    if (len < 1)
        return 0;
    if (!nkbevents) {
        *buf = KeyNoPress;
        return 0;
    }
    *buf = scancode_event_queue[--nkbevents];
    return 0;
}

void init_keyboard(void) {
    DeviceOps stdindev_ops = (DeviceOps){
        .read = &stdin_read,
        .write = &stdin_write,
        .open = &stdin_open,
        .close = &stdin_close,
    };
    DeviceOps kbdev_ops = (DeviceOps){
        .read = (void *)&kb_read,
        .write = &stdin_write,
        .open = &stdin_open,
        .close = &stdin_close,
    };
    mkdevice("/dev/stdin0", stdindev_ops);
    mkdevice("/dev/kb0", kbdev_ops);
    outb(PS2_DATA_REGISTER, 0xF4); // enable data reporting
    set_IDT_entry(33, (void *)keyboard_isr, 0x8E, kernel.IDT);
    map_ioapic(33, 1, 0, POLARITY_HIGH, TRIGGER_EDGE);
}

static void ps2_input_wait(void) {
	uint64_t timeout = 100000;
	while (timeout--) {
		if (!(inb(PS2_STATUS_REGISTER) & (1 << 1))) return;
	}
}

static void ps2_output_wait(void) {
	uint64_t timeout = 100000;
	while (timeout--) {
		if (inb(PS2_STATUS_REGISTER) & (1 << 0)) return;
	}
}

static uint8_t ps2_send_command_and_response(uint8_t cmd) {
    ps2_input_wait();
    outb(PS2_COMMAND_REGISTER, cmd);
    ps2_output_wait();
    return inb(PS2_DATA_REGISTER);
}

static void ps2_send_command_with_arg(uint8_t cmd, uint8_t arg) {
    ps2_input_wait();
    outb(PS2_COMMAND_REGISTER, cmd);
    ps2_input_wait();
    outb(PS2_DATA_REGISTER, arg);
}

typedef struct {
    bool left_click;
    bool right_click;
    int xmovement;
    int ymovement;
    bool ignoreme;
} MouseEvent;

MouseEvent mouse_events[256];
int num_mouse_events = 0;
__attribute__((interrupt))
void mouse_isr(void*) {
    static uint8_t mouse_packet[3];
    static int idx = 0;
    uint8_t data = inb(PS2_DATA_REGISTER);
    if (!idx && !(data & 0b1000)) goto ret;
    mouse_packet[idx++] = data;
    if (idx != 3) goto ret;
    // handle packet now that it's complete
    MouseEvent event;
    event.left_click  = mouse_packet[0] & 0b01;
    event.right_click = mouse_packet[0] & 0b10;
    if (mouse_packet[0] & (0b1 << 6)) {
        event.xmovement = (mouse_packet[0] & (0b1 << 4)) ? -255 : 255;
    } else {
        event.xmovement = (int8_t)mouse_packet[1];
    }
    if (mouse_packet[0] & (0b1 << 7)) {
        event.ymovement = (mouse_packet[0] & (0b1 << 5)) ? -255 : 255;
    } else {
        event.ymovement = (int8_t)mouse_packet[2];
    }
    event.ignoreme  = false;
    mouse_events[num_mouse_events++] = event;
    if (nkbevents > 255) {
        // shift everything back
        memmove(mouse_events, &mouse_events[1], 255);
        num_mouse_events--;
    }
    idx = 0;
ret:
    end_of_interrupt();
}

int mouse_read(void *f, MouseEvent *buf, size_t len, size_t off) {
    (void)f, (void)off;
    if (len < 1)
        return 0;
    if (!num_mouse_events) {
        buf->ignoreme = true;
        return 0;
    }
    *buf = mouse_events[--num_mouse_events];
    return 0;
}

void init_mouse(void) {
    DeviceOps mousedev_ops = (DeviceOps){
        .read = (void *)&mouse_read,
        .write = &stdin_write,
        .open = &stdin_open,
        .close = &stdin_close,
    };
    mkdevice("/dev/mouse0", mousedev_ops);
    ps2_send_command_with_arg(0xD4, 0xF4); // enable data reporting
    //ps2_send_command_with_arg(0xD4, 0xF6); // set defaults
    set_IDT_entry(44, (void *)mouse_isr, 0x8E, kernel.IDT);
    map_ioapic(44, 12, 0, POLARITY_HIGH, TRIGGER_EDGE);
}

static void disable_ps2_controller(void) {
    ps2_input_wait();
    outb(PS2_COMMAND_REGISTER, 0xAD); // port 1
    ps2_input_wait();
    outb(PS2_COMMAND_REGISTER, 0xA7); // port 2
}

static void enable_ps2_controller(void) {
    ps2_input_wait();
    outb(PS2_COMMAND_REGISTER, 0xAE); // port 1
    ps2_input_wait();
    outb(PS2_COMMAND_REGISTER, 0xA8); // port 2
}

static void set_ps2_config_and_disable_irqs(void) {
    uint8_t configb = ps2_send_command_and_response(PS2_CMD_GET_CONTROLLER_CONFIG);
    configb &= ~0b1;        // disable IRQs
    configb &= ~(0b1 << 4); // enable clock signal
    ps2_send_command_with_arg(PS2_CMD_SET_CONTROLLER_CONFIG, configb);
}

static void ps2_enable_irqs(void) {
    uint8_t configb = ps2_send_command_and_response(PS2_CMD_GET_CONTROLLER_CONFIG);
    configb |= (0b1 | 0b01); // enable IRQs for both devices
    ps2_send_command_with_arg(PS2_CMD_SET_CONTROLLER_CONFIG, configb);
}

static void ps2_check_dual_channels(void) {
    ps2_input_wait();
    outb(PS2_COMMAND_REGISTER, 0xA8); // try enable port 2
    uint8_t configb = ps2_send_command_and_response(PS2_CMD_GET_CONTROLLER_CONFIG);
    if (configb & (0b1 << 5)) {
        printf("PS/2 mouse not supported - not dual channel PS/2 controller\n");
        HALT_DEVICE();
    }
    ps2_input_wait();
    outb(PS2_COMMAND_REGISTER, 0xA7); // disable port 2 again
    configb &= ~((0b1 << 1) | (0b1 << 5)); // disable IRQs & enable clock port
    ps2_input_wait();
    outb(PS2_CMD_SET_CONTROLLER_CONFIG, configb);
}

static void ps2_reset_devices(void) {
    ps2_input_wait();
    outb(1, 0xFF); // port 1
    ps2_input_wait();
    outb(2, 0xFF); // port 2
}

void init_ps2(void) {
    disable_ps2_controller();
    ps2_output_wait();
    inb(PS2_DATA_REGISTER); // flush buffer
    set_ps2_config_and_disable_irqs();
    ps2_check_dual_channels();
    ps2_enable_irqs();
    enable_ps2_controller();
    ps2_reset_devices();
}




