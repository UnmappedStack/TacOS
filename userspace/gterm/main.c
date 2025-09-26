#include <stdlib.h>
#include <unistd.h>
#include <LibWM.h>
#include <string.h>
#include <pty.h>
#include <stdio.h>
#include <flanterm.h>
#include <flanterm_backends/fb.h>

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
        dup2(slave, 1);
        execve("/usr/bin/shell", (char*[]) {"shell", NULL}, environ);
    }
    for (;;) {
        char buf[256];
        if ((e=read(master, buf, 256)) > 0) {
            flanterm_write(ft_ctx, buf, e);
            lwm_flip_image(&win);
        }
    }
    return 0;
}
