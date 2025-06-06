#include <ctype.h>
#include <stdio.h>

int isalpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int toupper(char ch) {
    if (ch >= 'a' && ch <= 'z') return ch - ('a' - 'A');
    return ch;
}

int tolower(char ch) {
    if (ch >= 'A' && ch <= 'Z') return ch + ('a' - 'A');
    return ch;
}

int isspace(char ch) {
    return ch == ' ' ||
          ch == '\n' ||
          ch == '\r' ||
          ch == '\v' ||
          ch == '\f' ||
          ch == '\t';
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}
