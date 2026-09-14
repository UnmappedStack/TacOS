#pragma once
#include <stdint.h>

/* there will be one of this for every single physical base page frame
 * so it should be kept as small as possible. dont add to this unless
 * its really necessary. */
typedef struct {
    uint16_t refcount;
} PhysPage;

void pfndb_init(void);
