#pragma once

#define ENOENT 22

extern int errno;
int *__errno_location(void);
