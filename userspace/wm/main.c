#include <stdio.h>
#include <LibMedia.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <TacOS.h> // for keyboard stuff
#include <string.h>
#include <mman.h>
#include <stdlib.h>

#define WINDOW_BORDER_COLOUR  0x8dc1ee
#define WINDOW_BORDER_NOFOCUS 0x9acbf5
#define EXIT_BUTTON_COLOUR    0xc75050

typedef struct Window Window;

typedef enum {
    WIN_CREATE,
    WIN_SET_TITLE,
    WIN_FLIP_IMG,
} SrvCommand;

typedef enum {
    EVENT_RESPONSE,
} SrvEvent;

uint8_t open_window(Window *winlist, size_t x, size_t y, const char *title,
        size_t width, size_t height, uint32_t *imgbuf);
Window *get_window_by_id(Window *winlist, uint8_t id);
void set_win_title(Window *win, char *title, size_t slen);
void win_flip(Window *win);

void send_event(int fd, char *data, size_t data_len) {
    write(fd, data, data_len); // we kinda just hope for the best
}

void send_response(int fd, uint8_t cmd_id, uint8_t ret) {
    send_event(fd, (char[]) {4, EVENT_RESPONSE, cmd_id, ret}, 4);
}

void handle_command(int fd, Window *winlist) {
    uint8_t num_bytes;
    if (read(fd, &num_bytes, 1) < 1) return; // nothing to read
    if (num_bytes < 3) {
        fprintf(stderr, "Command and command ID aren't in packet (num_bytes=%u)\n", num_bytes);
        exit(-1);
    }
    uint8_t *packet = (char*) malloc(num_bytes + 1);
    packet[0] = num_bytes;
    if (read(fd, &packet[1], num_bytes-1) < num_bytes-1) {
        fprintf(stderr, "Got packet that claimed to be a bigger size than reality\n");
        // the things people will lie about these days... ):
        exit(-1);
    }
    SrvEvent cmd = packet[1];
    uint8_t cid = packet[2];
    Window *win;
    switch (cmd) {
    case WIN_CREATE:
        // format:
        // PACKSIZE | COMMAND | CMDID | WIDTH (x2) | HEIGHT (x2)
        uint16_t width  = packet[3] | ((uint16_t)packet[4] << 8);
        uint16_t height = packet[5] | ((uint16_t)packet[6] << 8);
        char buf[15];
        sprintf(buf, "wmsrvbuf%u", cid);
        int shmfd = shm_open(buf, O_CREAT, 0);
        ftruncate(shmfd, width * height * sizeof(uint32_t));
        uint32_t *imgbuf = mmap(NULL, width * height * sizeof(uint32_t), 0, MAP_SHARED, shmfd, 0);
        uint8_t wid = open_window(winlist, 50, 50, "", width+10, height, imgbuf);
        send_response(fd, cid, wid);
        break;
    case WIN_SET_TITLE:
        // format:
        // PACKSIZE | COMMAND | CMDID | WINID | (>1b) STRING
        win = get_window_by_id(winlist, packet[3]);
        char *t = &packet[4];
        set_win_title(win, t, num_bytes - 3);
        break;
    case WIN_FLIP_IMG:
        // format:
        // PACKSIZE | COMMAND | CMDID | WINID
        win = get_window_by_id(winlist, packet[3]);
        win_flip(win);
        break;
    default:
        fprintf(stderr, "Invalid command: %u\n", cmd);
        exit(-1);
    }
    free(packet);
}

void accept_commands(int *clients, size_t num_clients, Window *winlist) {
    for (size_t i = 0; i < num_clients; i++)
        handle_command(clients[i], winlist);
}

int server_init(void) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
err:
        fprintf(stderr, "Window server could not be started\n");
        return -1;
    }
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/winsrv");
    if (bind(fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) goto err;
    if (listen(fd, 10) < 0) goto err;
    printf("Successfully started window server listening at /winsrv\n");
    return fd;
}

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

struct Window {
    Window *next;
    size_t x, y;
    size_t width, height;
    char *title;
    bool focused;
    uint8_t wid;
    uint32_t *imgbuf;
    uint32_t *draw_from;
};

typedef struct {
    size_t x, y;
    bool clicking;
    bool ccm; // cursor control mode
    Window *windragging; // The window it's currently dragging (or NULL if none)
    size_t wdxoff, wdyoff; // offsets from top left of bar that we're dragging from (for windragging)
} Cursor;

Window *get_window_by_id(Window *winlist, uint8_t id) {
    while (winlist->next) {
        winlist = winlist->next;
        if (winlist->wid == id) return winlist;
    }
    fprintf(stderr, "window now found (invalid ID)\n");
    exit(-1);
}

Fb fb = {0};

void win_flip(Window *win) {
    memcpy(win->draw_from, win->imgbuf, win->width * win->height * sizeof(uint32_t));
}

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
    msync(fb.ptr, fb.pitch * fb.height, MS_SYNC);
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

void set_win_title(Window *win, char *title, size_t slen) {
    win->title = realloc(win->title, slen);
    strcpy(win->title, title);
}

uint8_t open_window(Window *winlist, size_t x, size_t y, const char *title, size_t width,
        size_t height, uint32_t *imgbuf) {
    static uint8_t wid = 0;
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
        .wid = wid,
        .imgbuf = imgbuf,
        .draw_from = malloc(width * height * sizeof(uint32_t)),
    };
    Window *newwin = (Window*) malloc(sizeof(Window));
    *newwin = new;
    last_window->next = newwin;
    return wid;
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
        size_t ax = 0;
        for (size_t j = x + 5; j < width + x - 5; j++, ax++) {
            where[j] = win->draw_from[i * (width-10) + ax];
        }
        where = (uint32_t*) ((uint8_t*) where + fb.pitch);
    }
    // draw title
    draw_text_bold(title, x + 10, y + 11, 0x00, fb.doublebuf, fb.width);

    // draw close button (doesn't do anything yet, just stylezzzz)
    where = (uint32_t*) (fb.doublebuf + (y+1) * fb.pitch);
    for (size_t i = 0; i < 33; i++) {
        for (size_t j = x + width - 60; j < width + x - 5; j++) {
            where[j] = EXIT_BUTTON_COLOUR;
        }
        where = (uint32_t*) ((uint8_t*) where + fb.pitch);
    }
    draw_text_bold("x", x + width - 37, y + 8, 0xFFFFFF, fb.doublebuf, fb.width);
}

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("TacOS HabaneroWM: Window Manager\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }
    
    int winsrv_fd = server_init();
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
    fb.ptr = mmap(NULL, fb.pitch * fb.height, 0, 0, fb.fd, 0);
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
    if ((bgpixels=decode_qoi("/media/bg.qoi", &bgwidth, &bgheight)) == NULL) return -1;
    printf("Successfully decoded background image\n");

    // load/decode cursor image
    uint32_t *cpixels;
    size_t cwidth, cheight;
    if ((cpixels=decode_qoi("/media/cursor.qoi", &cwidth, &cheight)) == NULL) return -1;
    printf("Successfully decoded cursor image\n");    
    
    // open test window
    
    int pid = fork();
    if (!pid)
        execve("/usr/bin/info", (char*[]) {"info", NULL}, (char*[]) {NULL});
    
    int *connected_clients = NULL;
    size_t num_clients = 0;
    for(;;) {
        cursor_getkey(&cursor, &winlist, kb_fd);
        int c;
        if ((c=accept_b(winsrv_fd, NULL, 0, false)) > 0) {
            connected_clients = realloc(connected_clients, ++num_clients * sizeof(int));
            connected_clients[num_clients-1] = c;
            printf("Established new connection from client\n");
        }

        accept_commands(connected_clients, num_clients, &winlist);

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
