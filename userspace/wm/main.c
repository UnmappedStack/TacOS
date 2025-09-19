#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>

typedef struct {
    int fd;
    uint64_t bpp;
    uint64_t pitch;
    uint32_t *ptr;
} Fb;

typedef struct {
    uint64_t pitch;
    uint32_t bpp;
} FbDevInfo;

int kb_fd;
Fb fb = {0};

void get_fb_info(int fd, uint64_t *pitchbuf, uint64_t *bppbuf) {
    FbDevInfo info = {0};
    if (read(fd, &info, sizeof(uint64_t) * 2) < 0) {
        printf("Failed to get framebuffer info.\n");
        exit(-1);
    }
    *pitchbuf = info.pitch, *bppbuf = info.bpp;
}

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("TacOS HabaneroWM: Window Manager\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }

    // map the framebuffer into vmem
    fb.fd = open("/dev/fb0", 0, 0);
    get_fb_info(fb.fd, &fb.pitch, &fb.bpp);
    if (fb.fd < 0) {
        printf("Failed to open framebuffer device.\n");
        exit(-1);
    }
    fb.ptr = mmap(NULL, 0, 0, 0, fb.fd, 0);
    printf("fb ptr = %p, pitch = %zu, bpp = %zu\n", fb.ptr, fb.pitch, fb.bpp);

    // load wallpaper
    int bg_x, bg_y, n;
    unsigned char *bgdata = stbi_load("/media/bg.png", &bg_x, &bg_y, &n, 0);
    stbi_image_free(bgdata);
}
