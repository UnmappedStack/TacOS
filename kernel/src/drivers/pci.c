#include <printf.h>
#include <kernel.h>
#include <mem/paging.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <io.h>

/* and also drivers which it needs to be able to handle the init of
 * if it finds said device in the PCI enumeration */
#include <nvme.h>

#define CONFIG_ADDR 0xCF8
#define CONFIG_DATA 0xCFC

uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg) {
    uint32_t lbus  = (uint32_t) bus;
    uint32_t ldev  = (uint32_t) device;
    uint32_t lfunc = (uint32_t) function;
    #define ENABLE (1 << 31)
    uint32_t addr = (uint32_t) (reg & 0xFC) | (lfunc << 8) | (ldev << 11) | (lbus << 16) | ENABLE;
    outl(CONFIG_ADDR, addr);
    return inl(CONFIG_DATA);
}

uint16_t pci_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg) {
    // `(reg & 2) * 8` will return 0 or 16, aka the offset of the read data to return
    uint32_t dword = pci_read32(bus, device, function, reg);
    return (uint16_t) (dword >> ((reg & 2) * 8) & 0xFFFF);
}

void pci_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg, uint32_t val) {
    uint32_t lbus  = (uint32_t) bus;
    uint32_t ldev  = (uint32_t) device;
    uint32_t lfunc = (uint32_t) function;
    #define ENABLE (1 << 31)
    uint32_t addr = (uint32_t) (reg & 0xFC) | (lfunc << 8) | (ldev << 11) | (lbus << 16) | ENABLE;
    outl(CONFIG_ADDR, addr);
    outl(CONFIG_DATA, val);
}

char *pci_vendorID_to_str(uint16_t id) {
    switch (id) {
    case 0x8086: return "Intel";
    case 0x1234: return "Qemu/Bochs";
    case 0x1022: return "AMD";
    default:
        return "Other vendor";
    };
}

uint16_t pci_get_vendorID(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor = pci_read16(bus, device, function, 0);
    return (vendor == 0xFFFF) ? 0 : vendor;
}

// returns a pointer to the capabilities list or a NULL pointer if the device doesn't
// implement a capabilities list
uint8_t pci_get_capabilities_list(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t status = pci_read16(bus, device, function, 6);
    if (!(status & 0b10000)) return 0;
    uint8_t capabilities_offset = ((uint8_t) pci_read16(bus, device, function, 0x34)) & ~0b11;
    return capabilities_offset;
}

typedef struct {
    uint64_t message_address;
    uint32_t message_data;
    struct {
        uint32_t masked;
    } vector_control;
} MSIXTableEntry;

// technically, MSI-X. Return BAR vaddr on success, 0 on failure.
uintptr_t pci_enable_msi(uint8_t bus, uint8_t device, uint8_t function,
        uint8_t capabilities_list, uint32_t handler_vector) {
    while (capabilities_list) {
        uint32_t this_entry = pci_read32(bus, device, function, capabilities_list);
        uint8_t capabilityID = (uint8_t) this_entry;
        uint8_t next = (uint8_t) (this_entry >> 8);
        if (capabilityID != 0x11) {
            capabilities_list = next;
            continue;
        }
        printf("MSI-X is supported!\n");
        uint8_t header_type = (uint8_t) pci_read16(bus, device, function, 0xE);
        if (header_type != 0 && header_type != 1) { // don't say this is dumb, it's
                                                    // for readability
            printf("Header type for enabling MSI must be 0 or 1.\n");
            return 0;
        }
        uint32_t buf = pci_read32(bus, device, function, capabilities_list + 4);

        uint8_t BIR = buf & 0b111;
        uint8_t BAR_offset = 0x10 + BIR * 4;
        uint32_t BAR_low = pci_read32(bus, device, function, BAR_offset);
        bool BAR_is_portIO = BAR_low & 0b1; // false if accessed through mmio
        if (BAR_is_portIO) {
            printf("BAR_is_portIO is true, not supported\n");
            return 0;
        }
        uint64_t BAR_addr;
        uint8_t BAR_type = (BAR_low & 0b110) >> 1;
        switch (BAR_type) {
        case 0x0:
            // 32 bit
            BAR_addr = BAR_low & 0xFFFFFFF0;
            break;
        case 0x1:
            // 16 bit (technically reserved in the latest PCI revisions)
            BAR_addr = BAR_low & 0xFFF0;
            break;
        case 0x2:
            // 64 bit
            uint32_t BAR_high = pci_read32(bus, device, function, BAR_offset + 4);
            BAR_addr = ((BAR_low & 0xFFFFFFF0) + (((uint64_t) BAR_high & 0xFFFFFFFF) << 32));
            break;
        }
        pci_write32(bus, device, function, BAR_offset, 0xffffffff);
        uint32_t BAR_size = ~pci_read32(bus, device, function, BAR_offset) + 1;
        pci_write32(bus, device, function, BAR_offset, BAR_low);
        uint32_t size_pages = PAGE_ALIGN_UP(BAR_size) / 4096;
        uintptr_t vaddr = valloc(size_pages);
        map_pages((void*) (kernel.cr3 + kernel.hhdm), vaddr, BAR_addr, size_pages,
                    KERNEL_PFLAG_WRITE | KERNEL_PFLAG_PRESENT);

        uint32_t table_offset = buf & ~0b111;
        MSIXTableEntry *msi_table = (MSIXTableEntry*) (vaddr + table_offset);
        msi_table[0].message_address = 0xFEE00000;
        msi_table[0].message_data    = handler_vector;
        msi_table[0].vector_control.masked = 0;
        // yeah I kinda did this the lazy way and ended up only mapping the first vector lol
        return vaddr;
    }
    printf("MSI-X support not found in capability list.\n");
    return 0;
}

void init_pci(void) {
    int bus, dev, func;
    for (bus = 0; bus < 256; bus++) {
        for (dev = 0; dev < 32; dev++) {
            uint16_t vendorID = pci_get_vendorID(bus, dev, 0);
            if (!vendorID) continue;
            for (func = 0; func < 8; func++) {
                vendorID = pci_get_vendorID(bus, dev, func);
                if (!vendorID) continue;
                uint16_t tmp = pci_read16(bus, dev, func, 10);
                uint8_t subclass = (uint8_t) tmp;
                uint8_t class = (uint8_t) (tmp >> 8);
                uint16_t devID = pci_read16(bus, dev, func, 2);
                uint8_t capabilities_list = pci_get_capabilities_list(bus, dev, func);
                char capabilities_list_str[20] = {0};
                uint64_to_hex_string(capabilities_list, &capabilities_list_str[2]);
                memcpy(capabilities_list_str, "0x", 2);
                printf("[bus:%i|dev:%i|func:%i] PCI device found:\n"
                       "    - Class %i, subclass %i\n"
                       "    - VendorID: 0x%x (%s)\n"
                       "    - DeviceID: 0x%x\n"
                       "    - Capabilities list: %s\n",
                       bus, dev, func,
                       class, subclass,
                       vendorID, pci_vendorID_to_str(vendorID),
                       devID,
                       (capabilities_list) ? capabilities_list_str : "None");
                if (class == 1 && subclass == 8)
                    nvme_init(bus, dev, func, capabilities_list);
            }
        }
    }
}
