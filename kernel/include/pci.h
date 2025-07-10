#pragma once
#include <stdint.h>

void init_pci(void);
uintptr_t pci_enable_msi(uint8_t bus, uint8_t device, uint8_t function,
        uint8_t capabilities_list, uint32_t handler_vector);
