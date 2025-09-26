#pragma once
#include <stdint.h>

typedef struct {
    int sockfd;
    uint8_t cid;
} LWMClient;

typedef struct {
    uint32_t *imgbuf;
    uint16_t width, height;
    uint8_t wid;
    LWMClient *client;
} LWMWindow;

typedef enum {
    WIN_CREATE,
    WIN_SET_TITLE,
    WIN_FLIP_IMG,
} SrvCommand;

typedef enum {
    EVENT_RESPONSE,
} SrvEvent;

int lwm_client_init(LWMClient *client);
int lwm_open_window(LWMClient *client, LWMWindow *win, uint16_t width, uint16_t height);
int lwm_set_window_title(LWMWindow *win, char *title);
int lwm_flip_image(LWMWindow *win);
