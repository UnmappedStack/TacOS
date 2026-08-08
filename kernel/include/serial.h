#pragma once
#include <isa/cpu.h>
#define COM1 0x3f8

void serial_init(void);
void write_serial_char(char ch);
void write_serial(const char *s);
