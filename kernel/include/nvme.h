#pragma once
#include <stdint.h>

void nvme_init(uint8_t bus, uint8_t device, uint8_t function, uint8_t capabilities_list);
