#include <framebuffer.h>
#include <kprintf.h>
#include <stddef.h>
#include <limine.h>

// the limine bootloader will detect this and fill it with the necessary info about 
// the avaliable framebuffers
static volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

// This should probably be in a global Kernel struct rather than by itself here (TODO)
// We store up to 3 framebuffers here (the first 3 that limine provides) and ignore the rest,
// as at this point in the boot sequence, there's no dynamic allocation yet.
// If you're wondering why I picked three, I pulled it outta my ass at complete random lol. I just
// need enough for both my laptop and monitor to show it so I needed more than 1.
static Framebuffer framebuffers[3] = {0};

void framebuffer_init(void) {
    struct limine_framebuffer_response *fb_response = fb_request.response;
    for (uint64_t i = 0; i < 4 && i < fb_response->framebuffer_count; i++) {
        framebuffers[i].addr          = fb_response->framebuffers[i]->address;
        framebuffers[i].width         = fb_response->framebuffers[i]->width;
        framebuffers[i].height        = fb_response->framebuffers[i]->height;
        framebuffers[i].pitch         = fb_response->framebuffers[i]->pitch;
        framebuffers[i].bytes_per_pix = fb_response->framebuffers[i]->bpp/8;
    }
    kprintf("Initiated %u framebuffers\n", fb_response->framebuffer_count);
}

// TODO: This should memset for efficiency
void framebuffer_draw_rect(int fb,
                            uint64_t x, uint64_t y, uint64_t width, uint64_t height,
                            uint32_t colour) {
    unsigned char *where =
        (unsigned char *)(((uint8_t *) framebuffers[fb].addr) +
                          y * framebuffers[fb].pitch) + x;
    uint8_t r = (colour >> 16) & 0xFF;
    uint8_t g = (colour >> 8) & 0xFF;
    uint8_t b = (colour) & 0xFF;
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            where[j * framebuffers[fb].bytes_per_pix + 0] = b;
            where[j * framebuffers[fb].bytes_per_pix + 1] = g;
            where[j * framebuffers[fb].bytes_per_pix + 2] = r;
        }
        where += framebuffers[fb].pitch;
    }
}

void fill_framebuffer(int fb, uint32_t colour) {
    framebuffer_draw_rect(fb, 0, 0, framebuffers[fb].width, framebuffers[fb].height, colour);
}
