#include <framebuffer.h>
#include <printf.h>
#include <tty.h>
#include <kernel.h>

void scroll_line() {
    kernel.tty.loc_y -= 32;
    scroll_pixels(32);
}

void newline() {
    kernel.tty.loc_x = 0;
    kernel.tty.loc_y += 16;
    if (kernel.tty.loc_y >= kernel.framebuffer.height - 16) scroll_line();
}

void write_framebuffer_char(char ch) {
    if (kernel.tty.loc_y >= kernel.framebuffer.height) {
        kernel.tty.loc_x = 0;
        kernel.tty.loc_y = 0;
    }
    if (ch == '\n') {
        newline();
        return;
    }
    draw_char(ch, kernel.tty.loc_x, kernel.tty.loc_y, FG_COLOUR);
    kernel.tty.loc_x += 8;
    if (kernel.tty.loc_x >= kernel.framebuffer.width) newline();
}

void write_framebuffer_text(const char *msg) {
    while (*msg) {
        write_framebuffer_char(*msg);
        msg++;
    }
}

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

void init_tty(void) {
    DeviceOps ttydev_ops = (DeviceOps) {
        .read = &fb_read,
        .write = &fb_write,
        .open = &fb_open,
        .close = &fb_close,
        .is_term = true,
    };
    mkdevice("/dev/tty0", ttydev_ops);
}
