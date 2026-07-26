#pragma once
#include <stdint.h>

typedef struct {
    void *addr;
    uint64_t width, height;
    uint64_t pitch;
    uint64_t bytes_per_pix;
} Framebuffer;

void framebuffer_init(void);
void framebuffer_draw_rect(int fb,
                            uint64_t x, uint64_t y, uint64_t width, uint64_t height,
                            uint32_t colour);
void fill_framebuffer(int fb, uint32_t colour);
