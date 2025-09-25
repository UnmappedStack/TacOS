#include <LibMedia.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
uint32_t *decode_qoi(const char *path, size_t *width, size_t *height) {
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
