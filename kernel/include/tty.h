#pragma once
#include <stddef.h>
#include <stdint.h>

#define MAX_ANSI_VALS 27 // valid ANSI escape codes cannot be longer than this

typedef enum { TTYEscape, TTYCSI, TTYNormal } TTYMode;

typedef struct {
    char cmd;
    size_t vals[MAX_ANSI_VALS];
    size_t nvals;
    char thisval[5];
    uint8_t thisvallen;
} ANSICmd;

typedef struct {
    TTYMode mode;
    size_t loc_x, loc_y;
    uint32_t fg_colour;
    uint32_t bg_colour;
    ANSICmd current_cmd;
} TTY;

void init_tty(void);
void write_framebuffer_text(const char *msg);
void write_framebuffer_char(char ch);
void write_framebuffer_char_nocover(char ch);
