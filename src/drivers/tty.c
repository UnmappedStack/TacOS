#include <framebuffer.h>
#include <printf.h>
#include <tty.h>
#include <kernel.h>

void scroll_line() {
    kernel.tty.loc_y -= 32;
    scroll_pixels(32);
}

void newline() {
    kernel.tty.loc_x = 0;
    kernel.tty.loc_y += 16;
    if (kernel.tty.loc_y >= kernel.framebuffer.height - 16) scroll_line();
}

void write_framebuffer_char(char ch) {
    if (kernel.tty.loc_y >= kernel.framebuffer.height) {
        kernel.tty.loc_x = 0;
        kernel.tty.loc_y = 0;
    }
    if (ch == '\n') {
        newline();
        return;
    }
    draw_char(ch, kernel.tty.loc_x, kernel.tty.loc_y, kernel.tty.fg_colour);
    kernel.tty.loc_x += 8;
    if (kernel.tty.loc_x >= kernel.framebuffer.width) newline();
}

void tty_set_cell_graphics_mode(ANSICmd *cmd) {
    // ANSI base colours except for default
    uint32_t tty_colours[] = {
        0x000000,
        0xFF0000,
        0x00FF00,
        0xFFFF00,
        0x0000FF,
        0xFF00FF,
        0x00FFFF,
        0xFFFFFF,
    };
    uint32_t fg_default = 0xd7dae0;
    uint32_t bg_default = 0x22262e;
    for (size_t i = 0; i < cmd->nvals; i++) {
        if (cmd->vals[i] >= 30 && cmd->vals[i] <= 39) {
            uint32_t col = (cmd->vals[i] == 39) ? fg_default : tty_colours[cmd->vals[i] - 30];
            kernel.tty.fg_colour = col;
        } else if (cmd->vals[i] >= 40 && cmd->vals[i] <= 49) {
            uint32_t col = (cmd->vals[i] == 49) ? bg_default : tty_colours[cmd->vals[i] - 40];
            kernel.tty.bg_colour = col;
        } else {
            printf("Graphics mode in ANSI not yet supported: %i\n", cmd->vals[i]);
        }
    }
}

void run_ansi_cmd(ANSICmd *cmd) {
    switch (cmd->cmd) {
    case 'H':
    case 'f':
        kernel.tty.loc_x = cmd->vals[0] * 8;
        kernel.tty.loc_y = cmd->vals[1] * 16;
        break;
    case 'J':
        // TODO: check how much of screen to clear
        fill_rect(0, 0, kernel.framebuffer.width, kernel.framebuffer.height, kernel.tty.bg_colour);
        kernel.tty.loc_x = 0;
        kernel.tty.loc_y = 0;
        break;
    case 'm':
        tty_set_cell_graphics_mode(cmd);
    default:
        printf("Unknown ANSI escape CSI mode command: %c\n", cmd->cmd);
    }
}

void escape_mode(char ch) {
    ANSICmd *cmd = &kernel.tty.current_cmd;
    if (ch == ';') {
        cmd->vals[cmd->nvals++] = str_to_u64(cmd->thisval);
        memset(cmd->thisval, 0, 5);
        cmd->thisvallen = 0;
    } else if (ch >= '0' && ch <= '9') {
        cmd->thisval[cmd->thisvallen++] = ch;
    } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
        // save previous arg
        cmd->vals[cmd->nvals++] = str_to_u64(cmd->thisval);
        memset(cmd->thisval, 0, 5);
        cmd->thisvallen = 0;
        // run command and go back to normal mode
        cmd->cmd = ch;
        run_ansi_cmd(cmd);
        kernel.tty.mode = TTYNormal;
    }
}

void write_tty_char(char ch) {
    switch (kernel.tty.mode) {
    case TTYNormal:
        if (ch == '\x1b' || ch == 27)
            kernel.tty.mode = TTYEscape;
        else
            write_framebuffer_char(ch);
        break;
    case TTYEscape:
        if (ch == '[') {
            kernel.tty.mode = TTYCSI;
            kernel.tty.current_cmd = (ANSICmd) {0};
        } else {
            printf("Only CSI mode ([) is supported in TTY0's ANSI parser (TODO)\n");
            kernel.tty.mode = TTYNormal;
        }
        break;
    case TTYCSI:
        escape_mode(ch);
        break;
    }
}

void write_framebuffer_text(const char *msg) {
    while (*msg) {
        write_tty_char(*msg);
        msg++;
    }
}

int fb_open(void *f) {
    (void) f;
    return 0;
}

int fb_close(void *f) {
    (void) f;
    return 0;
}

int fb_write(void *f, char *buf, size_t len, size_t off) {
    (void) f;
    if (!buf) return -1;
    buf = &buf[off];
    while (*buf && len) {
        write_tty_char(*buf);
        len--;
        buf++;
    }
    return 0;
}

int fb_read(void *f, char *buf, size_t len, size_t off) {
    (void) f;
    (void) buf;
    (void) len;
    (void) off;
    printf("Framebuffer device is write-only!\n");
    return -1;
}

void init_tty(void) {
    DeviceOps ttydev_ops = (DeviceOps) {
        .read = &fb_read,
        .write = &fb_write,
        .open = &fb_open,
        .close = &fb_close,
        .is_term = true,
    };
    mkdevice("/dev/tty0", ttydev_ops);
    kernel.tty.fg_colour = 0xd7dae0;
    kernel.tty.bg_colour = 0x22262e;
    kernel.tty.mode = TTYNormal;
    fill_rect(0, 0, kernel.framebuffer.width, kernel.framebuffer.height, kernel.tty.bg_colour);
}
