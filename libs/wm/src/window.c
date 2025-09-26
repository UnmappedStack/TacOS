#include <LibWM.h>
#include <mman.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>

// TODO: This should ideally store events in the way in a separate buffer to be handled
// later, not just skipped entirely
static uint8_t wait_for_response(int fd, uint8_t cid) {
    for (;;) {
        uint8_t sz;
        while (read(fd, &sz, 1) < 1);
        if (sz < 4) continue;
        char *packet = (char*) malloc(sz + 1);
        packet[0] = sz;
        if (read(fd, &packet[1], sz-1) < sz-1) exit(-1); // lied about size
        if (packet[1] != EVENT_RESPONSE || packet[2] != cid) continue;
        uint8_t ret = packet[3];
        free(packet);
        return ret;
    }
}
int lwm_client_init(LWMClient *client) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/winsrv");
    if (connect(fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        return -1;
    }
    client->sockfd = fd;
    client->cid = 0;
    return 0;
}

int lwm_open_window(LWMClient *client, LWMWindow *win, uint16_t width, uint16_t height) {
    uint8_t wl = (uint8_t)width;
    uint8_t wh = (uint8_t)(width >> 8);
    uint8_t hl = (uint8_t)height;
    uint8_t hh = (uint8_t)(height >> 8);
    if (write(client->sockfd, (uint8_t[]) {7, WIN_CREATE, client->cid, wl, wh, hl, hh}, 7) < 0) return -1;
    uint8_t resp = wait_for_response(client->sockfd, client->cid++);
    win->wid = resp;
    win->client = client;
    char shmfname[15];
    sprintf(shmfname, "wmsrvbuf%u", resp);
    int shmfd = shm_open(shmfname, 0, 0);
    if (shmfd < 0) return -1;
    win->imgbuf = mmap(NULL, sizeof(uint32_t) * width * height, 0, MAP_SHARED, shmfd, 0);
    return 0;
}

int lwm_set_window_title(LWMWindow *win, char *title) {
    uint8_t packet_len = strlen(title) + 5;
    uint8_t *cmd = (uint8_t*) malloc(packet_len + 1);
    memcpy(cmd, (uint8_t[]) {packet_len, WIN_SET_TITLE, win->client->cid, win->wid}, 4);
    strcpy(&cmd[4], title);
    if (write(win->client->sockfd, cmd, packet_len) < 0) -1;
    return 0;
}

int lwm_flip_image(LWMWindow *win) {
    if (write(win->client->sockfd, 
                (uint8_t[]) {4, WIN_FLIP_IMG, win->client->cid, win->wid}, 4) < 0) return -1;
    return 0;
}
