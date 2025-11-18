/*
 * Network Driver Integration for kernel
 * 
 * This file bridges the driver abstraction layer with the kernel's networking stack
 */

#include "../drivers/net_driver.h"

// External declarations (from net_registry.c)
extern const NetworkDriver* network_drivers[];
extern int num_network_drivers;

// Current active driver
static const NetworkDriver* active_driver = NULL;
static uint16_t active_io_base = 0;
static uint8_t active_bus = 0;
static uint8_t active_slot = 0;

// PCI Configuration Space access
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | 
                                  (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | 
                                  (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

// Initialize network drivers - scan PCI bus and try each driver
bool net_driver_init(void) {
    // Scan PCI bus for network cards
    for (uint8_t bus = 0; bus < 8; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vendor_device = pci_config_read(bus, slot, 0, 0);
            uint16_t vendor = vendor_device & 0xFFFF;
            uint16_t device = (vendor_device >> 16) & 0xFFFF;
            
            // Skip invalid devices
            if (vendor == 0xFFFF || vendor == 0x0000) continue;
            
            // Check each registered driver
            for (int d = 0; d < num_network_drivers; d++) {
                const NetworkDriver* driver = network_drivers[d];
                
                // Check if this driver supports the device
                for (int i = 0; i < driver->num_devices; i++) {
                    const DriverID* id = &driver->supported_devices[i];
                    
                    if (id->vendor_id == vendor && id->device_id == device) {
                        // Found a supported device!
                        // Get IO base address
                        uint32_t bar0 = pci_config_read(bus, slot, 0, 0x10);
                        uint16_t io_base = bar0 & ~0x3;
                        
                        // Enable bus mastering
                        uint32_t command = pci_config_read(bus, slot, 0, 0x04);
                        pci_config_write(bus, slot, 0, 0x04, command | 0x05);
                        
                        // Try to initialize
                        if (driver->init(io_base, bus, slot)) {
                            active_driver = driver;
                            active_io_base = io_base;
                            active_bus = bus;
                            active_slot = slot;
                            return true;
                        }
                    }
                }
            }
        }
    }
    
    return false;
}

// Get the name of the active driver
const char* net_driver_get_name(void) {
    if (!active_driver) return "None";
    return active_driver->name;
}

// Send a packet using the active driver
void net_driver_send_packet(const void* data, uint16_t length) {
    if (active_driver && active_driver->send) {
        active_driver->send(data, length);
    }
}

// Poll for packets using the active driver
void net_driver_poll_packets(void) {
    if (active_driver && active_driver->poll) {
        active_driver->poll();
    }
}

// Get MAC address from active driver
void net_driver_get_mac_address(uint8_t* mac) {
    if (active_driver && active_driver->get_mac) {
        active_driver->get_mac(mac);
    }
}

// Get link status from active driver
bool net_driver_link_up(void) {
    if (active_driver && active_driver->get_link_status) {
        return active_driver->get_link_status();
    }
    return false;
}

// Check if a driver is initialized
bool net_driver_is_initialized(void) {
    return active_driver != NULL;
}
