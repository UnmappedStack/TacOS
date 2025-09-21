#include <stdio.h>
#include "font.h"
#include <TacOS.h> // for keyboard stuff
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>

#define WINDOW_BORDER_COLOUR  0x8dc1ee
#define WINDOW_BORDER_NOFOCUS 0x9acbf5
#define EXIT_BUTTON_COLOUR    0xc75050

typedef struct {
    int fd;
    uint64_t bpp;
    uint64_t pitch;
    uint32_t *ptr;
    uint64_t width, height;
    void *doublebuf;
} Fb;

typedef struct {
    uint64_t pitch;
    uint32_t bpp;
    uint64_t width;
    uint64_t height;
} FbDevInfo;

typedef struct Window Window;
struct Window {
    Window *next;
    size_t x, y;
    size_t width, height;
    const char *title;
    bool focused;
};

typedef struct {
    size_t x, y;
    bool clicking;
    bool ccm; // cursor control mode
    Window *windragging; // The window it's currently dragging (or NULL if none)
    size_t wdxoff, wdyoff; // offsets from top left of bar that we're dragging from (for windragging)
} Cursor;

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
    uint32_t *location = (uint32_t *) (fb.doublebuf +
                                      y * fb.pitch);
    location[x] = colour;
}

static uint32_t big_to_little_endian_32(uint32_t n) {
    uint32_t byte0 = (n & 0xFF000000) >> 24;
    uint32_t byte1 = (n & 0x00FF0000) >> 8;
    uint32_t byte2 = (n & 0x0000FF00) << 8;
    uint32_t byte3 = (n & 0x000000FF) << 24;
    return byte0 | byte1 | byte2 | byte3;
}

typedef struct {
    char magic[4];
    uint32_t width;
    uint32_t height;
    uint8_t channels;
    uint8_t colourspace;
} __attribute__((packed)) QOIHeader;

#define IndexType     0x00
#define DiffType      0x40
#define LumaType      0x80
#define RunType       0xc0
#define RGBPixelType  0xfe
#define RGBAPixelType 0xff

typedef struct {
    uint8_t r, g, b, a;
} Pixel;

#define COLOR_HASH(x) (x.r*3 + x.g*5 + x.b*7 + x.a*11)
uint32_t *decode_image(const char *path, size_t *width, size_t *height) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "Failed to open %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *buf = malloc(len + 1);
    if (fread(buf, len, 1, f) == 0) {
        fprintf(stderr, "fread failed\n");
err:
        free(buf);
        fclose(f);
        return NULL;
    }
    QOIHeader *header = (QOIHeader*) buf;
    header->width  = big_to_little_endian_32(header->width);
    header->height = big_to_little_endian_32(header->height);
    if (memcmp(header->magic, "qoif", 4) != 0) {
        fprintf(stderr, "Invalid QOI file, either corrupt or not QOI\n");
        goto err;
    }
    printf(" === QOI Details: === \n"
           "  Width       |  %lu\n"
           "  Height      |  %lu\n"
           "  Channels    |  %u \n"
           "  Colourspace |  %u \n",
           (unsigned long) header->width, (unsigned long) header->height,
           header->channels, header->colourspace);

    printf("Starting image decode...\n");
    Pixel pixels[64] = {0};
    uint32_t *retpixels = (uint32_t*) malloc(header->width * header->height * 4);
    uint8_t *at = (uint8_t*) (buf + sizeof(QOIHeader));
    size_t run = 0; // of previous pixel
    Pixel pixel = {0};
    pixel.a = 255;
    for (size_t i = 0;; i++) {
        if (run > 0) {
            run--;
            goto add_to_buf;
        }
        if (len - (at - (uint8_t*)buf) >= 8 &&
            memcmp(at, "\0\0\0\0\0\0\0\1", 8) == 0) break;
        else if (*at == RGBPixelType) {
            pixel.r = at[1];
            pixel.g = at[2];
            pixel.b = at[3];
            at += 4;
        } else if (*at == RGBAPixelType) {
            pixel.r = at[1];
            pixel.g = at[2];
            pixel.b = at[3];
            pixel.a = at[4];
            at += 5;
        } else if ((*at & 0xc0) == IndexType) {
            pixel = pixels[*at];
            at++;
        } else if ((*at & 0xc0) == DiffType) {
            pixel.r += ((*at >> 4) & 0x03) - 2;
			pixel.g += ((*at >> 2) & 0x03) - 2;
			pixel.b += ( *at       & 0x03) - 2;
            at++;
        } else if ((*at & 0xc0) == LumaType) {
            int vg = (at[0] & 0x3f) - 32;
			pixel.r += vg - 8 + ((at[1] >> 4) & 0x0f);
			pixel.g += vg;
			pixel.b += vg - 8 + (at[1] & 0x0f);
            at += 2;
        } else if ((*at & 0xc0) == RunType) {
            run = (*at & 0x3f);
            at++;
        } else {
            fprintf(stderr, "Invalid chunk type\n");
            goto err;
        }
        pixels[COLOR_HASH(pixel) & (64 - 1)] = pixel;
add_to_buf:
        retpixels[i] = (pixel.r << 24) | (pixel.g << 16) | (pixel.b << 8) | pixel.a;
    }

    free(buf);
    fclose(f);
    *width = header->width;
    *height = header->height;
    return retpixels;
}

void draw_wallpaper(size_t bgwidth, size_t bgheight, uint32_t *bgpixels) {
    uint8_t *upto = (uint8_t*) fb.doublebuf;
    for (size_t y = 0; y < bgheight; y++) {
        if (y >= fb.height - 1) break;
        for (size_t x = 0; x < bgwidth; x++) {
            if (x > fb.width) break;
            ((uint32_t*) upto)[x] = bgpixels[y * bgwidth + x] >> 8;
        }
        upto += fb.pitch;
    }
}

void draw_cursor(size_t cwidth, size_t cheight, uint32_t *cpixels, size_t xs, size_t ys) {
    uint8_t *upto = ((uint8_t*) fb.doublebuf) + fb.pitch * ys;
    for (size_t y = 0; y < cheight; y++) {
        if (y >= fb.height - 1) break;
        for (size_t x = 0; x < cwidth; x++) {
            if (x > fb.width) break;
            size_t idx = y * cwidth + x;
            if (!(cpixels[idx] & 0xff)) continue; // skip transparent pixels
            ((uint32_t*) upto)[x + xs] = cpixels[idx] >> 8;
        }
        upto += fb.pitch;
    }
}

void doublebuf_swap(void) {
    memcpy(fb.ptr, fb.doublebuf, fb.pitch * fb.height);
}

Key getkey(int kb_fd) {
    Key key;
    if (read(kb_fd, &key, 1) < 0) return KeyNoPress;
    return key;
}

void handle_click(Window *winlist, Cursor *cursor) {
    if (cursor->clicking) {
        if (cursor->windragging) cursor->windragging = NULL;
        return;
    }
    // go through each window and check if it's within the coords, preferring top windows
    Window *last_window = winlist;
    Window *within_window = NULL;
    Window *within_window_prev = NULL;
    while (last_window->next) {
        Window *prev = last_window;
        last_window = last_window->next;
        if ( cursor->x >= last_window->x &&
             cursor->y >= last_window->y &&
             cursor->x <= last_window->x + last_window->width &&
             cursor->y <= last_window->y + last_window->height) {
            within_window = last_window;
            within_window_prev = prev;
        }
        last_window->focused = false; // anything else should be put out of focus
    }
    if (within_window == NULL) return;
    // set the window clicked as being in focus
    within_window->focused = true;
    // and bring it to the front
    within_window_prev->next = within_window->next;
    last_window = winlist;
    while (last_window->next) last_window = last_window->next;
    last_window->next = within_window;
    Window *second_last = last_window;
    within_window->next = NULL;

    // if it's within the close button area, close the window
    // (NOTE: this is temporary, later it should just send a signal to the program
    // via the window server so that the program responsible for the window can close it
    // itself)
    size_t close_butt_x = within_window->x + within_window->width - 60;
    size_t close_butt_y = within_window->y + 1;
    if ( cursor->x >= close_butt_x &&
         cursor->y >= close_butt_y &&
         cursor->x <= close_butt_x + 55 &&
         cursor->y <= close_butt_y + 33) {
        second_last->next = NULL;
        free(within_window);
        return;
    }

    // if it's clicking on the title bar, set the cursor to be dragging this window
    if (cursor->y <= within_window->y + 40) {
        cursor->windragging = within_window;
        cursor->wdxoff = cursor->x - within_window->x;
        cursor->wdyoff = cursor->y - within_window->y;
    }
}

void cursor_getkey(Cursor *cursor, Window *winlist, int kb_fd) {
#define speed 7
    Key key = getkey(kb_fd);
    if (key == KeyNoPress) return;
    if (key != KeySuper && !cursor->ccm) return;
    if ((key == CharL || key == CharJ || key == CharI || key == CharK)
            && cursor->windragging) {
        cursor->windragging->x = cursor->x - cursor->wdxoff;
        cursor->windragging->y = cursor->y - cursor->wdyoff;
    }
    switch (key) {
    case KeySuper:
        cursor->ccm = !cursor->ccm;
        return;
    case CharL:
        cursor->x += speed;
        return;
    case CharJ:
        cursor->x -= speed;
        return;
    case CharI:
        cursor->y -= speed;
        return;
    case CharK:
        cursor->y += speed;
        return;
    case KeySpace:
        handle_click(winlist, cursor);
        cursor->clicking = !cursor->clicking;
        return;
    }
}

void draw_char(char ch, uint64_t x_coord, uint64_t y_coord, uint32_t colour) {
    uint64_t first_byte_idx = ch * 16;
    for (size_t y = 0; y < 16; y++) {
        for (size_t x = 0; x < 8; x++) {
            if ((font[first_byte_idx + y] >> (7 - x)) & 1)
                draw_pixel(x_coord + x, y_coord + y, colour);
        }
    }
}

void draw_text(const char *s, uint64_t x, uint64_t y, uint32_t colour) {
    for (size_t i = 0; s[i]; i++) {
        draw_char(s[i], x + i * 9, y, colour);
    }
}

void draw_text_bold(const char *s, uint64_t x, uint64_t y, uint32_t colour) {
    draw_text(s, x, y, colour);
    draw_text(s, x + 1, y, colour);
    draw_text(s, x, y + 1, colour);
    draw_text(s, x + 1, y + 1, colour);
}

void open_window(Window *winlist, size_t x, size_t y, const char *title, size_t width, size_t height) {
    // find last window in queue
    Window *last_window = winlist;
    while (last_window->next) {
        last_window = last_window->next;
        last_window->focused = false;
    }
    // add to queue
    Window new = {
        .next = NULL,
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .title = title,
        .focused = true,
    };
    Window *newwin = (Window*) malloc(sizeof(Window));
    *newwin = new;
    last_window->next = newwin;
}

void draw_window(Window *win) {
    size_t x = win->x;
    size_t y = win->y;
    size_t width  = win->width;
    size_t height = win->height;
    const char *title  = win->title;
    // Draw rectangle for the main shape
    uint32_t *where = (uint32_t*) (fb.doublebuf + (y+1) * fb.pitch);
    for (size_t i = 0; i < height; i++) {
        for (size_t j = x; j < width + x; j++) {
            where[j] = (win->focused) ? WINDOW_BORDER_COLOUR : WINDOW_BORDER_NOFOCUS;
        }
        where = (uint32_t*) ((uint8_t*) where + fb.pitch);
    }

    // draw a thin white border
    uint32_t border_colour = 0xFFFFFF;
    //top
    where = (uint32_t*) (fb.doublebuf + y * fb.pitch);
    for (size_t i = x; i < width + x; i++)
        where[i] = border_colour;
    //bottom
    where = (uint32_t*) (fb.doublebuf + (y+height+1) * fb.pitch);
    for (size_t i = x; i < width + x; i++)
        where[i] = border_colour;
    // sides
    where = (uint32_t*) (fb.doublebuf + y * fb.pitch);
    for (size_t i = y; i < height + y + 1; i++) {
        where[x-1    ] = border_colour;
        where[x+width] = border_colour;
        where = (uint32_t*) ((uint8_t*) where + fb.pitch);
    }

    // draw the area where the frame would be (temp)
    where = (uint32_t*) (fb.doublebuf + (y+41) * fb.pitch);
    for (size_t i = 0; i < height - 46; i++) {
        for (size_t j = x + 5; j < width + x - 5; j++) {
            where[j] = 0xFFFFFF;
        }
        where = (uint32_t*) ((uint8_t*) where + fb.pitch);
    }

    // draw title
    draw_text_bold(title, x + 25, y + 11, 0x00);

    // draw close button (doesn't do anything yet, just stylezzzz)
    where = (uint32_t*) (fb.doublebuf + (y+1) * fb.pitch);
    for (size_t i = 0; i < 33; i++) {
        for (size_t j = x + width - 60; j < width + x - 5; j++) {
            where[j] = EXIT_BUTTON_COLOUR;
        }
        where = (uint32_t*) ((uint8_t*) where + fb.pitch);
    }
    draw_text_bold("x", x + width - 37, y + 8, 0xFFFFFF);
}

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("TacOS HabaneroWM: Window Manager\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }
    
    Cursor cursor = { .x=5, .y=5, .clicking=false, .ccm=false,
                        .windragging=NULL, .wdxoff=0, .wdyoff=0 };
    Window winlist;
    winlist.next = NULL;

    // map the framebuffer into mem
    fb.fd = open("/dev/fb0", 0, 0);
    get_fb_info(fb.fd, &fb.pitch, &fb.bpp);
    if (fb.fd < 0) {
        printf("Failed to open framebuffer device.\n");
        exit(-1);
    }
    fb.ptr = mmap(NULL, 0, 0, 0, fb.fd, 0);
    fb.doublebuf = (void*) malloc(fb.height * fb.pitch);
    printf("fb ptr = %p, pitch = %zu, bpp = %zu\n", fb.ptr, fb.pitch, fb.bpp);
    
    // open keyboard device etc for non blocking IO keyboard events
    int kb_fd = open("/dev/kb0", 0, 0);
    if (kb_fd < 0) {
        printf("Failed to open keyboard device.\n");
        exit(-1);
    }

    // load/decode background image
    uint32_t *bgpixels;
    size_t bgwidth, bgheight;
    if ((bgpixels=decode_image("/media/bg.qoi", &bgwidth, &bgheight)) == NULL) return -1;
    printf("Successfully decoded background image\n");

    // load/decode cursor image
    uint32_t *cpixels;
    size_t cwidth, cheight;
    if ((cpixels=decode_image("/media/cursor.qoi", &cwidth, &cheight)) == NULL) return -1;
    printf("Successfully decoded cursor image\n");    
    
    // open test window
    open_window(&winlist, 50, 50, "Test Window", 500, 300);
    open_window(&winlist, 300, 200, "Test Window 2", 500, 300);

    for(;;) {
        cursor_getkey(&cursor, &winlist, kb_fd);
        draw_wallpaper(bgwidth, bgheight, bgpixels);

        Window *at = &winlist;
        while (at->next) {
            at = at->next;
            draw_window(at);
        }

        draw_cursor(cwidth, cheight, cpixels, cursor.x, cursor.y);
        if (cursor.clicking) {
            for (size_t i = 0; i < 5; i++) {
                for (size_t j = 0; j < 5; j++)
                    draw_pixel(cursor.x - 2 + j, cursor.y - 2 + i, 0xFF0000);
            }
        }
        doublebuf_swap();
    }
}
