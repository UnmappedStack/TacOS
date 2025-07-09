#include <printf.h>
#include <io.h>
#include <stdint.h>

#define CONFIG_ADDR 0xCF8
#define CONFIG_DATA 0xCFC

uint16_t pci_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg) {
    uint32_t lbus  = (uint32_t) bus;
    uint32_t ldev  = (uint32_t) device;
    uint32_t lfunc = (uint32_t) function;
    #define ENABLE (1 << 31)
    uint32_t addr = (uint32_t) (reg & 0xFC) | (lfunc << 8) | (ldev << 11) | (lbus << 16) | ENABLE;
    outl(CONFIG_ADDR, addr);
    // `(reg & 2) * 8` will return 0 or 16, aka the offset of the read data to return
    uint32_t read_back = inl(CONFIG_DATA);
    return (uint16_t) (read_back >> ((reg & 2) * 8) & 0xFFFF);
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

void init_pci(void) {
    int bus, dev, func;
    for (bus = 0; bus < 256; bus++) {
        for (dev = 0; dev < 32; dev++) {
            for (func = 0; func < 8; func++) {
                uint16_t vendorID = pci_get_vendorID(bus, dev, func);
                if (!vendorID) continue;
                uint16_t tmp = pci_read16(bus, dev, func, 10);
                uint8_t subclass = (uint8_t) tmp;
                uint8_t class = (uint8_t) (tmp >> 8);
                uint16_t devID = pci_read16(bus, dev, func, 2);
                printf("[bus:%i|dev:%i|func:%i] PCI device found:\n"
                       "    - VendorID: 0x%x (%s)\n"
                       "    - DeviceID: 0x%x\n"
                       "    - Class: %i\n"
                       "    - Subclass: %i\n",
                       bus, dev, func,
                       vendorID, pci_vendorID_to_str(vendorID),
                       devID,
                       class, subclass);
            }
        }
    }
    for (;;);
}
