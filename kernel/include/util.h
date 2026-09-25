#pragma once
#include <isa/cpu.h>

// gets pointer of struct of type `type` given struct element pointer `ptr` of element name `member`
#define CONTAINER_OF(ptr, type, member) ((type*)((void*)ptr - offsetof(type, member)))
int count_leading_zeroes(uint64_t x);

// a lot of this maths stuff is acquired from boron with relatively
// minor modifications:
// https://github.com/iProgramMC/Boron
typedef int64_t FixedPoint;
#define FIXED_POINT 16
#define INT_TO_FP(x) ((FixedPoint)(x) << FIXED_POINT)
#define FP_TO_INT(x) ((int)((x) >> FIXED_POINT))
int rand(void);
FixedPoint cos(int angle);
FixedPoint sin(int angle);
