#pragma once

#define INT_BITS   (sizeof(int) * CHAR_BIT)
#define INT_MAX    ((int)((1U << (INT_BITS - 1)) - 1))
#define INT_MIN    (-INT_MAX - 1)

#define SHRT_BITS  (sizeof(short) * CHAR_BIT)
#define SHRT_MAX   ((short)((1U << (SHRT_BITS - 1)) - 1))
#define SHRT_MIN   (-SHRT_MAX - 1)

#define CHAR_BIT   8
