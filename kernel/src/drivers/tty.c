#include <tty.h>
#include <serial.h>
#include <kernel.h>
#include <string.h>
#include <kprintf.h>
#include <framebuffer.h>
#include <assets.h>

// TODO: this should probably store a buffer
void draw_char_at(int fb, int x, int y, int colour, int bg, unsigned char ch) {
    static_assert(FONT_WIDTH == 8, "font width should probably change to allow larger font sizes (but width 8 is required rn)");
    for (int down = 0; down < FONT_HEIGHT; down++) {
        uint8_t char_byte = font[(FONT_HEIGHT * ch) + down];
        for (int across = 0; across < FONT_WIDTH; across++) {
            uint8_t char_bit_set = (char_byte >> ((FONT_WIDTH-1)-across)) & 1;
            if (char_bit_set)
                framebuffer_draw_pixel(fb, x+across, y+down, colour);
            else
                framebuffer_draw_pixel(fb, x+across, y+down, bg);
        }
    }
}

void scroll_pixels(int fb, size_t num_pix) {
    Framebuffer *framebuffer = &kernel_info.framebuffers[fb];
    size_t max_height = framebuffer->height - num_pix;
    uintptr_t new_row_loc = (uintptr_t)framebuffer->addr;
    uintptr_t old_row_loc = (uintptr_t)framebuffer->addr +
                            (num_pix * framebuffer->pitch);
    for (size_t y = 0; y < max_height; y++) {
        memcpy((uint32_t *)new_row_loc, (uint32_t *)old_row_loc,
               framebuffer->bytes_per_pix*framebuffer->width);
        new_row_loc += framebuffer->pitch;
        old_row_loc += framebuffer->pitch;
    }
    framebuffer_draw_rect(fb, 0, max_height, framebuffer->width, num_pix, BG_DEFAULT);
}

void scroll_lines(int fb, int num_lines) {
    int *y = &kernel_info.framebuffers[fb].tty.cursor_y;
    *y -= num_lines;
    scroll_pixels(fb, FONT_HEIGHT * num_lines);
}

void tty_draw_char(int colour, int bg, unsigned char ch) {
    for (int fb = 0; fb < kernel_info.num_framebuffers; fb++) {
        Framebuffer *buf = &kernel_info.framebuffers[fb];
        if (!buf->addr) continue;
        int *x = &buf->tty.cursor_x;
        int *y = &buf->tty.cursor_y;
        if (*y >= buf->tty.chars_height) scroll_lines(fb, 8);
        if (ch == '\n') {
            *x = 0;
            (*y)++;
            continue;
        }
        draw_char_at(fb, (*x)++ * FONT_WIDTH, *y * FONT_HEIGHT, colour, bg, ch);
        if (*x >= buf->tty.chars_width) {
            *x = 0;
            (*y)++;
        }
    }
}

void tty_set_cell_graphics_mode(TTYCmd cmd) {
    // ANSI base colours except for default
    int tty_colours[] = {
        0x000000, /*red*/ 0x853A3C, /*green*/ 0x72854D, /*yellow*/ 0xE9B771,
        /*blue*/ 0x588193, /*magenta*/0x9A6C91, 0x00FFFF, 0xFFFFFF,
    };
    for (int arg = 0; arg < cmd.num_args; arg++) {
        if (cmd.args[arg] >= 90 && cmd.args[arg] <= 97)
            cmd.args[arg] -= 90 - 30;
        else if (cmd.args[arg] >= 100 && cmd.args[arg] <= 107)
            cmd.args[arg] -= 100 - 40;

        if (cmd.args[arg] >= 30 && cmd.args[arg] <= 39) {
            uint32_t col = (cmd.args[arg] == 39)
                               ? FG_DEFAULT
                               : tty_colours[cmd.args[arg] - 30];
            kernel_info.tty_state.fg_colour = col;
        } else if (cmd.args[arg] >= 40 && cmd.args[arg] <= 49) {
            uint32_t col = (cmd.args[arg] == 49)
                               ? BG_DEFAULT
                               : tty_colours[cmd.args[arg] - 40];
            kernel_info.tty_state.bg_colour = col;
        } else {
            kernel_info.tty_state.bg_colour = BG_DEFAULT;
            kernel_info.tty_state.fg_colour = FG_DEFAULT;
        }
    }
}

void run_ansi_cmd(TTYCmd cmd) {
    switch (cmd.cmd) {
    case 'm':
        tty_set_cell_graphics_mode(cmd);
        break;
    default:
        write_serial("unknown ansi command\n");
    }
}

void escape_mode(unsigned char ch) {
    TTYCmd *cmd = &kernel_info.tty_state.cmd;
    if (ch >= '0' && ch <= '9') {
        // digit of an argument
        cmd->current_arg[cmd->current_arg_len++] = ch;
    } else if (ch == ';') {
        // end of an argument, turn it to an int and save it
        cmd->args[cmd->num_args++] = str_to_u64(cmd->current_arg);
        cmd->current_arg_len = 0;
    } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
        // save last argument first
        cmd->args[cmd->num_args++] = str_to_u64(cmd->current_arg);
        cmd->current_arg_len = 0;
        // then execute the command
        cmd->cmd = ch;
        kernel_info.tty_state.state = StateNormal;
        run_ansi_cmd(*cmd);
        cmd->num_args = 0;
    }
}

void tty_write_char(unsigned char ch) {
    if (!kernel_info.tty_state.init_complete) {
        kernel_info.tty_state.fg_colour = FG_DEFAULT;
        kernel_info.tty_state.bg_colour = BG_DEFAULT;
        kernel_info.tty_state.init_complete = true;
    }
    switch (kernel_info.tty_state.state) {
    case StateNormal:
        if (ch == '\x1b' || ch == '\e') {
            kernel_info.tty_state.state = StateEscape;
            return;
        }
        tty_draw_char(kernel_info.tty_state.fg_colour, kernel_info.tty_state.bg_colour, ch);
        break;
    case StateEscape:
        if (ch != '[') return; // only csi is supported rn
        kernel_info.tty_state.state = StateCSI;
        return;
    case StateCSI:
        escape_mode(ch);
        return;
    default: ;;
    }
}

void tty_write_text(const char *s) {
    for (; *s; s++) {
        tty_write_char(*s);
    }
}
