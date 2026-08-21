#include <tty.h>
#include <kernel.h>
#include <string.h>
#include <kprintf.h>
#include <framebuffer.h>
#include <assets.h>

// TODO; this should probably store a buffer
void draw_char_at(int fb, int x, int y, int colour, unsigned char ch) {
    (void) ascii_art; // make the compiler happy

    static_assert(FONT_WIDTH == 8, "font width should probably change to allow larger font sizes (but width 8 is required rn)");
    for (int down = 0; down < FONT_HEIGHT; down++) {
        uint8_t char_byte = font[(FONT_HEIGHT * ch) + down];
        for (int across = 0; across < FONT_WIDTH; across++) {
            uint8_t char_bit_set = (char_byte >> ((FONT_WIDTH-1)-across)) & 1;
            if (char_bit_set)
                framebuffer_draw_pixel(fb, x+across, y+down, colour);
            else
                framebuffer_draw_pixel(fb, x+across, y+down, 0);
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
    framebuffer_draw_rect(fb, 0, max_height, framebuffer->width, num_pix, 0);
}

void scroll_lines(int fb, int num_lines) {
    int *y = &kernel_info.framebuffers[fb].tty.cursor_y;
    *y -= num_lines;
    scroll_pixels(fb, FONT_HEIGHT * num_lines);
}

void tty_draw_char(int colour, unsigned char ch) {
    // TODO/FIXME: temporary and bad thing to ignore ansi which is kinda broken.
    // Actually implement an ANSI state machine.
    static int num_in_ansi = 0;
    if (ch == '\e') {
        num_in_ansi = 6;
        return;
    }
    if (num_in_ansi) {
        num_in_ansi--;
        return;
    }

    // actual char drawing, after the disgusting ansi skip thing
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
        draw_char_at(fb, (*x)++ * FONT_WIDTH, *y * FONT_HEIGHT, colour, ch);
        if (*x >= buf->tty.chars_width) {
            *x = 0;
            (*y)++;
        }
    }
}

void tty_write_text(int colour, const char *s) {
    for (; *s; s++) {
        tty_draw_char(colour, *s);
    }
}
