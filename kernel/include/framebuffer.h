#pragma once
#include <stdint.h>

// We store up to 3 framebuffers (the first 3 that limine provides) and ignore the rest,
// as at this point in the boot sequence, there's no dynamic allocation yet.
// If you're wondering why I picked three, I pulled it outta my ass at complete random lol. I just
// need enough for both my laptop and monitor to show it so I needed more than 1.
#define MAX_FRAMEBUFFERS 3

// also contains info for the tty of each framebuffer
typedef struct {
    void *addr;
    uint64_t width, height;
    uint64_t pitch;
    uint64_t bytes_per_pix;

    struct {
        int cursor_x, cursor_y;
        int chars_width, chars_height;
    } tty;
} Framebuffer;

void framebuffer_init(void);
void framebuffer_draw_rect(int fb,
                            uint64_t x, uint64_t y, uint64_t width, uint64_t height,
                            uint32_t colour);
void fill_framebuffer(int fb, uint32_t colour);
void framebuffer_draw_pixel(int fb, uint64_t x, uint64_t y, uint32_t colour);
void tty_write_text(int colour, char *s);
