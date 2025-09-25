#include <LibWM.h>
#include <LibMedia.h>
#include <stdlib.h>

uint32_t *buf;
uint16_t winwidth;
static void draw_pixel(int x, int y, uint32_t colour) {
    buf[y * winwidth + x] = colour;
}

int main(int argc, char **argv) {
    LWMClient client;
    LWMWindow win;
    uint16_t width  = 300;
    uint16_t height = 350;
    if (lwm_client_init(&client) < 0) return -1;
    if (lwm_open_window(&client, &win, width, height) < 0) return -1;
    if (lwm_set_window_title(&win, "Info") < 0) return -1;
    buf = win.imgbuf, winwidth = width;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            win.imgbuf[y * width + x] = 0xFDFBD4;
        }
    }
    size_t imgwidth, imgheight;
    uint32_t *icon = decode_qoi("/media/chilli.qoi", &imgwidth, &imgheight);
    if (!icon) return -1;
    size_t x_off = (width / 2) - (imgwidth / 2) - 5;
    for (int y = 0; y < imgheight; y++) {
        for (int x = 0; x < imgwidth; x++) {
            uint32_t pixel = icon[y * imgwidth + x];
            if (!(pixel & 0xFF)) continue; // transparent pixels
            win.imgbuf[(y+10) * width + (x_off+x)] = pixel >> 8;
        }
    }
    free(icon);
    const char *txt = "TacOS x86_64, version 0.45 beta\n\n"
                      "This project is under MPL 2.0, licensed by UnmappedStack.\n"
                      "See LICENSE in the source repo for more information:\n\n"
                      "https://github.com/\nUnmappedStack/TacOS";
    draw_text_wrap(txt, 10, 20 + imgheight, 0, draw_pixel, width - 10);
    return 0;
}
