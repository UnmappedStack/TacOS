#include <LibWM.h>

int main(int argc, char **argv) {
    LWMClient client;
    LWMWindow win;
    uint16_t width  = 500;
    uint16_t height = 300;
    if (lwm_client_init(&client) < 0) return -1;
    if (lwm_open_window(&client, &win, width, height) < 0) return -1;
    if (lwm_set_window_title(&win, "Test Window") < 0) return -1;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            win.imgbuf[y * width + x] = 0x00FF00;
        }
    }
    return 0;
}
