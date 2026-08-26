#pragma once
#include <stdint.h>

#define FG_DEFAULT 0xC9CACA
#define BG_DEFAULT 0x121418
#define MAX_ANSI_ARGS 27 // compliant ansi shouldnt surpass this i think

typedef enum {
    StateNormal, StateEscape, StateCSI
} TTYState;

typedef struct {
    char current_arg[5];
    int current_arg_len;

    uint64_t args[MAX_ANSI_ARGS];
    int num_args;

    unsigned char cmd;
} TTYCmd;

typedef struct {
    int fg_colour, bg_colour;
    TTYState state;
    TTYCmd cmd;
    bool init_complete;
} GlobalTTYState;

#define FONT_WIDTH   8
#define FONT_HEIGHT 16
void draw_char_at(int fb, int x, int y, int colour, int bg, unsigned char ch);
void tty_draw_char(int colour, int bg, unsigned char ch);
void tty_write_text(const char *s);
void tty_write_char(unsigned char ch);
