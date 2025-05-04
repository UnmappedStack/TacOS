#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

double fabs(double x) {
    uint64_t mask = 0x7FFFFFFFFFFFFFFF;
    uint64_t bits = *(uint64_t*)&x;
    bits &= mask;
    return *(double*)&bits;
}

double log(double x) {
    if (x <= 0.0) return 0.0;
    double y = (x - 1) / (x + 1);
    double y2 = y * y;
    double result = 0.0;
    double term = y;
    for (int n = 1; n < 20; n += 2) {
        result += term / n;
        term *= y2;
    }
    return 2.0 * result;
}

double exp(double x) {
    double sum = 1.0;
    double term = 1.0;
    for (int i = 1; i < 30; ++i) {
        term *= x / i;
        sum += term;
    }
    return sum;
}

double pow(double x, double y) {
    return exp(y * log(x));
}

double ldexp(double x, int exp) {
    return x * pow(2, exp);
}

double floor(double x) {
    return (double) ((int64_t) x);
}

double strtod(const char *nptr, char **endptr) {
    (void) nptr, (void) endptr;
    printf("Whatever program it is called strtod... don't. Be more like Doom. Stop expecting stuff from math.h to just be there for you. Take this stub for whatever port I'm doing now and leave.\n"
           "(TODO: not implemented yet in libc: strtod)\n");
    exit(-1);
}

double frexp(double x, int *exp) {
    (void) x, (void) exp;
    printf("TODO: frexp is not implemented yet in libc\n");
    exit(-1);
}

double fmod(double x, double y) {
    double n = floor(x / n);
    return x - n * y;
}
