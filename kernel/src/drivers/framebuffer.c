#include <framebuffer.h>
#include <tty.h>
#include <kprintf.h>
#include <stddef.h>
#include <limine.h>
#include <kernel.h>

// the limine bootloader will detect this and fill it with the necessary info about 
// the avaliable kernel_info.framebuffers
static volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

void framebuffer_init(void) {
    struct limine_framebuffer_response *fb_response = fb_request.response;
    kernel_info.num_framebuffers = fb_response->framebuffer_count;
    for (int i = 0; i < MAX_FRAMEBUFFERS && i < kernel_info.num_framebuffers; i++) {
        kernel_info.framebuffers[i].addr          = fb_response->framebuffers[i]->address;
        kernel_info.framebuffers[i].width         = fb_response->framebuffers[i]->width;
        kernel_info.framebuffers[i].height        = fb_response->framebuffers[i]->height;
        kernel_info.framebuffers[i].pitch         = fb_response->framebuffers[i]->pitch;
        kernel_info.framebuffers[i].bytes_per_pix = fb_response->framebuffers[i]->bpp/8;

        kernel_info.framebuffers[i].tty.cursor_x
            = kernel_info.framebuffers[i].tty.cursor_y
            = 0;
        kernel_info.framebuffers[i].tty.chars_width  = kernel_info.framebuffers[i].width  / FONT_WIDTH;
        kernel_info.framebuffers[i].tty.chars_height = kernel_info.framebuffers[i].height / FONT_HEIGHT;
    }
    kprintf("Initiated %u framebuffer(s)\n", fb_response->framebuffer_count);
}

void framebuffer_draw_rect(int fb,
                            uint64_t x, uint64_t y, uint64_t width, uint64_t height,
                            uint32_t colour) {
    unsigned char *where =
        (unsigned char *)(((uint8_t *) kernel_info.framebuffers[fb].addr) +
                          y * kernel_info.framebuffers[fb].pitch) + x;
    uint8_t r = (colour >> 16) & 0xFF;
    uint8_t g = (colour >> 8) & 0xFF;
    uint8_t b = (colour) & 0xFF;
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            where[j * kernel_info.framebuffers[fb].bytes_per_pix + 0] = b;
            where[j * kernel_info.framebuffers[fb].bytes_per_pix + 1] = g;
            where[j * kernel_info.framebuffers[fb].bytes_per_pix + 2] = r;
        }
        where += kernel_info.framebuffers[fb].pitch;
    }
}

void fill_framebuffer(int fb, uint32_t colour) {
    framebuffer_draw_rect(fb, 0, 0, kernel_info.framebuffers[fb].width, kernel_info.framebuffers[fb].height, colour);
}

void framebuffer_draw_pixel(int fb, uint64_t x, uint64_t y, uint32_t colour) {
    uint32_t *where =
        (uint32_t*)(((uint8_t *) kernel_info.framebuffers[fb].addr) +
                          y * kernel_info.framebuffers[fb].pitch) + x;
    *where = colour;
}
