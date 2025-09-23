#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define DEF_WIDTH  500
#define DEF_HEIGHT 300

typedef enum {
    WIN_CREATE,
    WIN_SET_TITLE,
} SrvCommand;

typedef enum {
    EVENT_RESPONSE,
} SrvEvent;

uint8_t cid = 0;

uint8_t wait_for_response(int fd, uint8_t cid) {
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

uint8_t open_window(int fd) {
    if (write(fd, (uint8_t[]) {3, WIN_CREATE, cid}, 3) < 0) exit(-1);
    uint8_t resp = wait_for_response(fd, cid);
    cid++;
    return resp;
}

void win_set_title(int fd, uint8_t wid, char *title) {
    uint8_t packet_len = strlen(title) + 5;
    uint8_t *cmd = (uint8_t*) malloc(packet_len + 1);
    memcpy(cmd, (uint8_t[]) {packet_len, WIN_SET_TITLE, cid, wid}, 4);
    strcpy(&cmd[4], title);
    if (write(fd, cmd, packet_len) < 0) exit(-1);
}

int main(int argc, char **argv) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
err:
        fprintf(stderr, "Failed to connect to window server\n");
        return -1;
    }
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/winsrv");
    if (connect(fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) goto err;
    printf("Successfully connected to window server\n");
    uint32_t *imgbuf = (uint32_t*) malloc(sizeof(uint32_t) * DEF_WIDTH * DEF_HEIGHT);
    for (size_t i = 0; i < DEF_WIDTH * DEF_HEIGHT; i++)
        imgbuf[i] = 0xFF0000;
    uint8_t wid = open_window(fd);
    win_set_title(fd, wid, "Title set by client :)");
    return 0;
}
