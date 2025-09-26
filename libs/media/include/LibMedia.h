#pragma once
#include <stdint.h>
#include <stddef.h>

// Image
uint32_t *decode_qoi(const char *path, size_t *width, size_t *height);

// Font
void draw_char(char ch, uint64_t x_coord, uint64_t y_coord, uint32_t colour, uint32_t *buf, int width);
void draw_text(const char *s, uint64_t x, uint64_t y, uint32_t colour, uint32_t *buf, int width);
void draw_text_bold(const char *s, uint64_t x, uint64_t y, uint32_t colour, uint32_t *buf, int width);
void draw_text_wrap(const char *s, uint64_t x, uint64_t y, uint32_t colour, uint32_t *buf, int width, int max_width);
