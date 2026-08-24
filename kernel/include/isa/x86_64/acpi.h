#pragma once
#include <stdint.h>

typedef struct {
    char signature[8];
    uint8_t checksum;
    char OEMID[6];
    uint8_t revision;
    uint32_t rsdt_address;

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t rsvd[3];
} __attribute__((packed)) RSDP;

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
} __attribute__((packed)) ISDTHeader;

//technically either rsdt OR xsdt
typedef struct {
    ISDTHeader header;
    uint64_t entries[0];
} __attribute__((packed)) XSDT;

void acpi_init(void);
void *find_MADT(XSDT *root_rsdt);
