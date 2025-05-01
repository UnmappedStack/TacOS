// OS specific stuff for TacOS in the libc
#pragma once

typedef enum {
    CharA, CharB, CharC, CharD, CharE, CharF, CharG, CharH, CharI, CharJ, CharK,
    CharL, CharM, CharN, CharO, CharP, CharQ, CharR, CharS, CharT, CharU, CharV,
    CharW, CharX, CharY, CharZ,
    Dec0, Dec1, Dec2, Dec3, Dec4, Dec5, Dec6, Dec7, Dec8, Dec9,
    KeyEnter, KeyShift, KeySpace, KeyForwardSlash, KeyBackSlash, KeyComma,
    KeySingleQuote, KeySemiColon, KeyLeftSquareBracket, KeyRightSquareBracket,
    KeyEquals, KeyMinus, KeyBackTick, KeyAlt, KeySuper, KeyTab,
    KeyCapsLock, KeyEscape, KeyBackspace, KeyLeftArrow, KeyRightArrow,
    KeyUpArrow, KeyDownArrow, KeyRelease, KeyUnknown, KeyNoPress,
} Key;

Key getkey_nonblocking();
