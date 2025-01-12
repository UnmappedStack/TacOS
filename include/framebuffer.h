#pragma once

typedef struct {
    void *addr;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint64_t bytes_per_pix;
} Framebuffer;

void init_framebuffer();
void write_framebuffer_text(const char *msg);
void write_framebuffer_char(char ch);
