#include <string.h>
#include <printf.h>
#include <cpu.h>
#include <font.h>
#include <stdint.h>
#include <framebuffer.h>
#include <kernel.h>

static volatile struct limine_framebuffer_request limine_framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0, 
};

Framebuffer boot_get_framebuffer() {
    if(!limine_framebuffer_request.response) {
        printf("No framebuffer found! Halting.\n");
        HALT_DEVICE();
    }
    return (Framebuffer) {
        .addr          = (*limine_framebuffer_request.response->framebuffers)->address,
        .width         = (*limine_framebuffer_request.response->framebuffers)->width,
        .height        = (*limine_framebuffer_request.response->framebuffers)->height,
        .pitch         = (*limine_framebuffer_request.response->framebuffers)->pitch,
        .bytes_per_pix = (*limine_framebuffer_request.response->framebuffers)->bpp / 8,
        .width_bytes   = (*limine_framebuffer_request.response->framebuffers)->width * (*limine_framebuffer_request.response->framebuffers)->bpp / 8,
    };
}

void draw_pixel(uint64_t x, uint64_t y, uint32_t colour) {
    uint32_t *location = (uint32_t*)(((uint8_t*) kernel.framebuffer.addr) + y * kernel.framebuffer.pitch);
    location[x] = colour;
}

void fill_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint32_t colour) {
    unsigned char *where = (unsigned char*)(((uint8_t*) kernel.framebuffer.addr) + y * kernel.framebuffer.pitch) + x;
    uint8_t r = (colour >> 16) & 0xFF;
    uint8_t g = (colour >> 8 ) & 0xFF;
    uint8_t b = (colour      ) & 0xFF;
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            where[j*kernel.framebuffer.bytes_per_pix] = b;
            where[j*kernel.framebuffer.bytes_per_pix + 1] = g;
            where[j*kernel.framebuffer.bytes_per_pix + 2] = r;
        }
        where += kernel.framebuffer.pitch;
    }

}

void draw_char_nocover(char ch, uint64_t x_coord, uint64_t y_coord, uint32_t colour) {
    uint64_t first_byte_idx = ch * 16;
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 8; x++) {
            if ((font[first_byte_idx + y] >> (7 - x)) & 1)
                draw_pixel(x_coord + x, y_coord + y, colour);
        }
    }
}

void draw_char(char ch, uint64_t x_coord, uint64_t y_coord, uint32_t colour) {
    uint64_t first_byte_idx = ch * 16;
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 8; x++) {
            if ((font[first_byte_idx + y] >> (7 - x)) & 1)
                draw_pixel(x_coord + x, y_coord + y, colour);
            else
                draw_pixel(x_coord + x, y_coord + y, kernel.tty.bg_colour);
        }
    }
}

void scroll_pixels(size_t num_pix) {
    size_t max_height = kernel.framebuffer.height - num_pix;
    uintptr_t new_row_loc = (uintptr_t) kernel.framebuffer.addr;
    uintptr_t old_row_loc = (uintptr_t) kernel.framebuffer.addr + (num_pix * kernel.framebuffer.pitch);
    for (size_t y = 0; y < max_height; y++) {
        memcpy((uint32_t*) new_row_loc, (uint32_t*) old_row_loc, kernel.framebuffer.width_bytes);
        new_row_loc += kernel.framebuffer.pitch;
        old_row_loc += kernel.framebuffer.pitch;
    }
    fill_rect(0, max_height, kernel.framebuffer.width, num_pix, kernel.tty.bg_colour);
}

int fbdevopen(void *f) {(void)f;return 0;}
int fbdevclose(void *f) {(void)f;return 0;}
void init_framebuffer() {
    kernel.framebuffer = boot_get_framebuffer();
    // raw framebuffer device
    DeviceOps fbdev_ops = {.open=&fbdevopen,.close=&fbdevclose,.is_term=false};
    mkdevice("/dev/fb0", fbdev_ops);
    // Fill the screen and finish up
    printf("Framebuffer initialised.\n");
}
