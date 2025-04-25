#pragma once
#include <stdint.h>
#include <stddef.h>

#define BG_COLOUR 0x22262e
#define FG_COLOUR 0xd7dae0

typedef struct {
    void *addr;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint64_t bytes_per_pix;
    uint64_t width_bytes;
} Framebuffer;

void init_framebuffer();
void scroll_pixels(size_t num_pix);
void draw_char(char ch, uint64_t x_coord, uint64_t y_coord, uint32_t colour);
