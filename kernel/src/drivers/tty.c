#include <tty.h>
#include <kernel.h>
#include <kprintf.h>
#include <framebuffer.h>
#include <assets.h>

void draw_char_at(int fb, int x, int y, int colour, unsigned char ch) {
    (void) ascii_art; // make the compiler happy

    static_assert(FONT_WIDTH == 8, "font width should probably change to allow larger font sizes (but width 8 is required rn)");
    for (int down = 0; down < FONT_HEIGHT; down++) {
        uint8_t char_byte = font[(FONT_HEIGHT * ch) + down];
        for (int across = 0; across <= FONT_WIDTH; across++) {
            uint8_t char_bit_set = (char_byte >> (FONT_WIDTH-across)) & 1;
            if (!char_bit_set) continue;
            framebuffer_draw_pixel(fb, x+across, y+down, colour);
        }
    }
}

void tty_draw_char(int colour, unsigned char ch) {
    for (int fb = 0; fb < kernel_info.num_framebuffers; fb++) {
        Framebuffer *buf = &kernel_info.framebuffers[fb];
        int *x = &buf->tty.cursor_x;
        int *y = &buf->tty.cursor_y;
        if (ch == '\n') {
            *x = 0;
            (*y)++;
            continue;
        }
        draw_char_at(fb, (*x)++ * FONT_WIDTH, *y, colour, ch);
        if (*x >= buf->tty.chars_width) *x = 0;
        if (*y >= buf->tty.chars_height) kpanic("(TODO: scrolling)");
    }
}

void tty_write_text(int colour, char *s) {
    for (; *s; s++) {
        tty_draw_char(colour, *s);
    }
}
