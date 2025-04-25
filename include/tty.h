#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t loc_x, loc_y;
    uint32_t fg_colour;
    uint32_t bg_colour;
} TTY;

void init_tty(void);
void write_framebuffer_text(const char *msg);
void write_framebuffer_char(char ch);
