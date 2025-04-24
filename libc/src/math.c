#include <stdint.h>

double fabs(double x) {
    uint64_t mask = 0x7FFFFFFFFFFFFFFF;
    uint64_t bits = *(uint64_t*)&x;
    bits &= mask;
    return *(double*)&bits;
}
