#include <stdlib.h>
#include <time.h>
#include <TacOS.h>
#include <unistd.h>
#include <LibWM.h>
#include <string.h>
#include <pty.h>
#include <stdio.h>
#include <flanterm.h>
#include <flanterm_backends/fb.h>

void pause(void) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500;
    nanosleep(&ts);
}

extern char **environ;
int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == '-') {
        printf("GTerm: A simple graphical terminal emulator.\n"
               "Copyright (C) 2025 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0. "
               "See LICENSE in the source repo for more information.\n");
        exit(-1);
    }
    LWMClient client;
    LWMWindow win;
    uint16_t width  = 500;
    uint16_t height = 300;
    if (lwm_client_init(&client) < 0) return -1;
    if (lwm_open_window(&client, &win, width, height) < 0) return -1;
    if (lwm_set_window_title(&win, "GTerm") < 0) return -1;
    int master, slave, e;
    e=openpty(&master, &slave, NULL, NULL, NULL);
    if (e < 0) {
        fprintf(stderr, "Failed to open pty\n");
        return -1;
    }
    struct flanterm_context *ft_ctx = flanterm_fb_init(
        NULL,
        NULL,
        win.imgbuf, width, height, width * sizeof(uint32_t),
        8, 16,
        8, 8,
        8, 0,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0
    );
    if (!ft_ctx) return -1;
    int pid = fork();
    if (!pid) {
        dup2(slave, 0);
        dup2(slave, 1);
        dup2(slave, 2);
        execve("/usr/bin/shell", (char*[]) {"shell", NULL}, environ);
    }
    char kbbuf[256] = {0};
    size_t kbidx = 0;
    bool caps = false;
    for (;;) {
        uint8_t buf[256];
        if ((e=read(master, buf, 256)) > 0) {
            flanterm_write(ft_ctx, buf, e);
            lwm_flip_image(&win);
        }
        if (lwm_get_event(&client, buf) < 0) continue;
        if (buf[1] != EVENT_KEYPRESS) continue;
        Key key = (buf[3]) | ((uint16_t)buf[4]<<8);
        if (key <= CharZ) {
            char start = (caps) ? 'A' : 'a';
            char ch = start + key;
            kbbuf[kbidx++] = ch;
            flanterm_write(ft_ctx, &ch, 1);
            lwm_flip_image(&win);
        } else if (key == KeyCapsLock)
            caps = !caps;
        else if (key == KeyBackspace && kbidx) {
            kbidx--;
            const char *back_seq = "\x1b[1D \x1b[1D";
            flanterm_write(ft_ctx, back_seq, strlen(back_seq));
            lwm_flip_image(&win);
        } else if (key == KeyEnter) {
            kbbuf[kbidx++] = '\0';
            for (int i = 0; i < 2; i++) pause();
            write(master, kbbuf, kbidx);
            kbidx = 0;
        }
    }
    return 0;
}
