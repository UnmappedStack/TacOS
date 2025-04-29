#pragma once
#include <stdint.h>

typedef struct {
    char signature[8];
    uint8_t checksum;
    char OEMID[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__ ((packed)) RSDP;

typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char OEM_id[6];
    char OEM_table_id[8];
    uint32_t OEM_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__ ((packed)) ISDTHeader;

typedef struct {
    ISDTHeader header;
    uint32_t entries[0];
} __attribute__ ((packed)) RSDT;

void init_acpi(void);
void* find_MADT(RSDT *root_rsdt);
