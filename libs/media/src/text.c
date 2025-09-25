#include <LibMedia.h>
#include <bmfont.h>

void draw_char(char ch, uint64_t x_coord, uint64_t y_coord, uint32_t colour, void *draw_pixel_fn) {
    uint64_t first_byte_idx = ch * 16;
    void (*draw_pixel)(int, int, int) = (void (*)(int, int, int))draw_pixel_fn;
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 8; x++) {
            if ((font[first_byte_idx + y] >> (7 - x)) & 1)
                draw_pixel(x_coord + x, y_coord + y, colour);
        }
    }
}

void draw_text(const char *s, uint64_t x, uint64_t y, uint32_t colour, void *draw_pixel_fn) {
    for (size_t i = 0; s[i]; i++) {
        draw_char(s[i], x + i * 9, y, colour, draw_pixel_fn);
    }
}

void draw_text_bold(const char *s, uint64_t x, uint64_t y, uint32_t colour, void *draw_pixel_fn) {
    draw_text(s, x, y, colour, draw_pixel_fn);
    draw_text(s, x + 1, y, colour, draw_pixel_fn);
    draw_text(s, x, y + 1, colour, draw_pixel_fn);
    draw_text(s, x + 1, y + 1, colour, draw_pixel_fn);
}

void draw_text_wrap(const char *s, uint64_t x, uint64_t y, uint32_t colour, void *draw_pixel_fn, int max_width) {
    uint64_t x_start = x;
    for (size_t i = 0; s[i]; i++) {
        if (x + 9 >= max_width || s[i] == '\n') {
            x = x_start, y += 16;
            if (s[i] == '\n') continue;
        }
        draw_char(s[i], x, y, colour, draw_pixel_fn);
        x += 9;
    }
}
