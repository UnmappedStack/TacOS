#include <string.h>
#include <printf.h>
#include <cpu.h>
#include <font.h>
#include <stdint.h>
#include <framebuffer.h>
#include <kernel.h>

#define BG_COLOUR 0x22262e
#define FG_COLOUR 0xd7dae0

int fb_open(void *f) {
    (void) f;
    return 0;
}

int fb_close(void *f) {
    (void) f;
    return 0;
}

int fb_write(void *f, char *buf, size_t len, size_t off) {
    (void) f;
    if (!buf) return -1;
    buf = &buf[off];
    while (*buf && len) {
        write_framebuffer_char(*buf);
        len--;
        buf++;
    }
    return 0;
}

int fb_read(void *f, char *buf, size_t len, size_t off) {
    (void) f;
    (void) buf;
    (void) len;
    (void) off;
    printf("Framebuffer device is write-only!\n");
    return -1;
}

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

void draw_char(char ch, uint64_t x_coord, uint64_t y_coord, uint32_t colour) {
    uint64_t first_byte_idx = ch * 16;
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 8; x++) {
            if ((font[first_byte_idx + y] >> (7 - x)) & 1)
                draw_pixel(x_coord + x, y_coord + y, colour);
            else
                draw_pixel(x_coord + x, y_coord + y, BG_COLOUR);
        }
    }
}

void scroll_pixels(size_t num_pix) {
    size_t max_height = kernel.framebuffer.height - num_pix;
    size_t fb_width_bytes = kernel.framebuffer.width * kernel.framebuffer.bytes_per_pix;
    uintptr_t new_row_loc = (uintptr_t) kernel.framebuffer.addr;
    for (size_t y = 0; y < max_height; y++) {
        uint32_t *old_row_loc = (uint32_t*)(((uint8_t*) kernel.framebuffer.addr) + (y + num_pix) * kernel.framebuffer.pitch);
        memcpy((uint32_t*) new_row_loc, old_row_loc, fb_width_bytes);
        new_row_loc += kernel.framebuffer.pitch;
    }
    fill_rect(0, max_height, kernel.framebuffer.width, num_pix, BG_COLOUR);
}

void scroll_line() {
    kernel.char_y -= 16;
    scroll_pixels(16);
}

void newline() {
    kernel.char_x = 0;
    kernel.char_y += 16;
    if (kernel.char_y >= kernel.framebuffer.height - 16) scroll_line();
}

void write_framebuffer_char(char ch) {
    if (kernel.char_y >= kernel.framebuffer.height) {
        kernel.char_x = 0;
        kernel.char_y = 0;
    }
    if (ch == '\n') {
        newline();
        return;
    }
    draw_char(ch, kernel.char_x, kernel.char_y, FG_COLOUR);
    kernel.char_x += 8;
    if (kernel.char_x >= kernel.framebuffer.width) newline();
}

void write_framebuffer_text(const char *msg) {
    while (*msg) {
        write_framebuffer_char(*msg);
        msg++;
    }
}

void init_framebuffer() {
    kernel.framebuffer = boot_get_framebuffer();
    // tty device
    DeviceOps ttydev_ops = (DeviceOps) {
        .read = &fb_read,
        .write = &fb_write,
        .open = &fb_open,
        .close = &fb_close,
        .is_term = true,
    };
    mkdevice("/dev/tty0", ttydev_ops);
    // raw framebuffer device
    DeviceOps fbdev_ops = {0};
    fbdev_ops.is_term = true;
    mkdevice("/dev/fb0", ttydev_ops);
    // Fill the screen and finish up
    fill_rect(0, 0, kernel.framebuffer.width, kernel.framebuffer.height, BG_COLOUR);
    printf("Framebuffer initialised.\n");
}
