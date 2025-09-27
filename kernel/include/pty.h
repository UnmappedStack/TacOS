#pragma once
#include <ringbuffer.h>
#include <stddef.h>

typedef struct PtyDev PtyDev;
struct PtyDev {
    PtyDev *other;  // points to master if this is slave & vice versa
    RingBuffer data;
    bool is_master;
};

void init_usrptys(void);
