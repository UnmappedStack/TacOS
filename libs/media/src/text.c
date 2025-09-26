#include <LibMedia.h>
#include <bmfont.h>

void draw_char(char ch, uint64_t x_coord, uint64_t y_coord, uint32_t colour, uint32_t *buf, int width) {
    uint64_t first_byte_idx = ch * 16;
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 8; x++) {
            if (!((font[first_byte_idx + y] >> (7 - x)) & 1)) continue;
            int xa = x_coord + x;
            int ya = y_coord + y;
            buf[ya * width + xa] = colour;
        }
    }
}

void draw_text(const char *s, uint64_t x, uint64_t y, uint32_t colour, uint32_t *buf, int width) {
    for (size_t i = 0; s[i]; i++) {
        draw_char(s[i], x + i * 9, y, colour, buf, width);
    }
}

void draw_text_bold(const char *s, uint64_t x, uint64_t y, uint32_t colour, uint32_t *buf, int width) {
    draw_text(s, x, y, colour, buf, width);
    draw_text(s, x + 1, y, colour, buf, width);
    draw_text(s, x, y + 1, colour, buf, width);
    draw_text(s, x + 1, y + 1, colour, buf, width);
}

void draw_text_wrap(const char *s, uint64_t x, uint64_t y, uint32_t colour, uint32_t *buf, int width, int max_width) {
    uint64_t x_start = x;
    for (size_t i = 0; s[i]; i++) {
        if (x + 9 >= max_width || s[i] == '\n') {
            x = x_start, y += 16;
            if (s[i] == '\n') continue;
        }
        draw_char(s[i], x, y, colour, buf, width);
        x += 9;
    }
}
