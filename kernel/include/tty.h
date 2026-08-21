#pragma once

#define FONT_WIDTH   8
#define FONT_HEIGHT 16
void draw_char_at(int fb, int x, int y, int colour, unsigned char ch);
void tty_draw_char(int colour, unsigned char ch);
