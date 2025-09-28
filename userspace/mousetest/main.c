#include <stdio.h>
#include <mman.h>
#include <stdlib.h>
#include <string.h>
#include <TacOS.h>

typedef struct {
    int fd;
    uint64_t bpp;
    uint64_t pitch;
    uint32_t *ptr;
    uint64_t width, height;
} Fb;

typedef struct {
    uint64_t pitch;
    uint32_t bpp;
    uint64_t width;
    uint64_t height;
} FbDevInfo;
Fb fb = {0};

void get_fb_info(int fd, uint64_t *pitchbuf, uint64_t *bppbuf) {
    FbDevInfo info;
    if (read(fd, &info, sizeof(uint64_t) * 4) < 0) {
        printf("Failed to get framebuffer info.\n");
        exit(-1);
    }
    *pitchbuf = info.pitch, *bppbuf = info.bpp;
    fb.width = info.width, fb.height = info.height;
}

void draw_pixel(uint64_t x, uint64_t y, uint32_t colour) {
    uint32_t *location = (uint32_t *) (fb.ptr +
                                      y * fb.pitch);
    location[x] = colour;
}

int main(int argc, char **argv) {
    size_t x, y;
    uint32_t colour = 0xFF0000;
    x = y = 20;
    fb.fd = open("/dev/fb0", 0, 0);
    if (fb.fd < 0) {
        printf("Failed to open framebuffer device.\n");
        exit(-1);
    }
    get_fb_info(fb.fd, &fb.pitch, &fb.bpp);
    fb.ptr = mmap(NULL, fb.pitch * fb.height, 0, 0, fb.fd, 0);

    printf("fb ptr = %p, pitch = %zu, bpp = %zu\n", fb.ptr, fb.pitch, fb.bpp);
    int f = open("/dev/mouse0", 0, 0);
    if (!f) {
        printf("failed to open mouse\n");
        return -1;
    }
    for (;;) {
        MouseEvent event;
        read(f, &event, 1);
        if (event.ignoreme) continue;
        // redraw cos state has changed
        memset(fb.ptr, 0xff, fb.pitch * fb.height);
        for (size_t xa = x; xa < x+5; xa++) {
            for (size_t ya = y; ya < y+5; ya++) 
                draw_pixel(xa, ya, colour);
        }
    }
}
