/*
 * Intel e1000 Gigabit Ethernet Driver for slopOS
 * Intel 82540EM, 82545EM, 82574L (QEMU default)
 */

#include "net_driver.h"

// Intel e1000 PCI IDs
#define E1000_VENDOR_ID 0x8086
#define E1000_DEV_82540EM 0x100E
#define E1000_DEV_82545EM 0x100F
#define E1000_DEV_82574L 0x10D3
#define E1000_DEV_82543GC 0x1004  // Desktop
#define E1000_DEV_82544EI 0x1008  // Server
#define E1000_DEV_82544GC 0x1009  // Desktop
#define E1000_DEV_82541PI 0x1076  // Desktop
#define E1000_DEV_82547GI 0x1075  // Desktop
#define E1000_DEV_82571EB 0x105E  // Dual port
#define E1000_DEV_82572EI 0x107D  // Server
#define E1000_DEV_82573E 0x108B   // Mobile
#define E1000_DEV_82573L 0x109A   // Mobile

// e1000 Registers (memory-mapped)
#define E1000_CTRL 0x0000      // Device Control
#define E1000_STATUS 0x0008    // Device Status
#define E1000_EECD 0x0010      // EEPROM Control
#define E1000_EERD 0x0014      // EEPROM Read
#define E1000_ICR 0x00C0       // Interrupt Cause Read
#define E1000_IMS 0x00D0       // Interrupt Mask Set
#define E1000_RCTL 0x0100      // Receive Control
#define E1000_TCTL 0x0400      // Transmit Control
#define E1000_RDBAL 0x2800     // RX Descriptor Base Low
#define E1000_RDBAH 0x2804     // RX Descriptor Base High
#define E1000_RDLEN 0x2808     // RX Descriptor Length
#define E1000_RDH 0x2810       // RX Descriptor Head
#define E1000_RDT 0x2818       // RX Descriptor Tail
#define E1000_TDBAL 0x3800     // TX Descriptor Base Low
#define E1000_TDBAH 0x3804     // TX Descriptor Base High
#define E1000_TDLEN 0x3808     // TX Descriptor Length
#define E1000_TDH 0x3810       // TX Descriptor Head
#define E1000_TDT 0x3818       // TX Descriptor Tail
#define E1000_RAL 0x5400       // Receive Address Low
#define E1000_RAH 0x5404       // Receive Address High

// Control bits
#define E1000_CTRL_RST 0x04000000   // Device reset
#define E1000_CTRL_SLU 0x00000040   // Set link up
#define E1000_RCTL_EN 0x00000002    // Receive enable
#define E1000_RCTL_BAM 0x00008000   // Broadcast accept
#define E1000_TCTL_EN 0x00000002    // Transmit enable
#define E1000_TCTL_PSP 0x00000008   // Pad short packets

// Descriptor sizes
#define E1000_NUM_RX_DESC 16
#define E1000_NUM_TX_DESC 8

// I/O operations
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

// RX Descriptor
struct e1000_rx_desc {
    uint32_t addr_low;
    uint32_t addr_high;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

// TX Descriptor
struct e1000_tx_desc {
    uint32_t addr_low;
    uint32_t addr_high;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));

// Driver state
static struct {
    uint32_t mem_base;
    uint8_t mac[ETH_ALEN];
    struct e1000_rx_desc rx_descs[E1000_NUM_RX_DESC] __attribute__((aligned(16)));
    struct e1000_tx_desc tx_descs[E1000_NUM_TX_DESC] __attribute__((aligned(16)));
    uint8_t rx_buffers[E1000_NUM_RX_DESC][2048] __attribute__((aligned(16)));
    uint8_t tx_buffers[E1000_NUM_TX_DESC][2048] __attribute__((aligned(16)));
    uint16_t rx_tail;
    uint16_t tx_tail;
    bool initialized;
} e1000_state;

// Read/Write MMIO registers
static uint32_t e1000_read(uint32_t reg) {
    volatile uint32_t* addr = (volatile uint32_t*)(e1000_state.mem_base + reg);
    return *addr;
}

static void e1000_write(uint32_t reg, uint32_t val) {
    volatile uint32_t* addr = (volatile uint32_t*)(e1000_state.mem_base + reg);
    *addr = val;
}

// Read EEPROM
static uint16_t e1000_eeprom_read(uint8_t addr) {
    e1000_write(E1000_EERD, (1U << 0) | ((uint32_t)addr << 8));
    uint32_t tmp;
    while (!((tmp = e1000_read(E1000_EERD)) & (1U << 4)));
    return (uint16_t)((tmp >> 16) & 0xFFFF);
}

// PCI config space access (same as in net_integration.c)
static uint32_t pci_config_read_local(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | 
                                  (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(0xCF8, address);
    return inl(0xCFC);
}

// Initialize e1000
static bool e1000_init(uint16_t io_base, uint8_t bus, uint8_t slot) {
    (void)io_base; // e1000 uses MMIO, not port I/O
    
    // Read BAR0 from PCI config space (memory-mapped I/O address)
    uint32_t bar0 = pci_config_read_local(bus, slot, 0, 0x10);
    
    // Check if it's a memory BAR (bit 0 should be 0)
    if (bar0 & 0x1) {
        return false; // This is an I/O port BAR, not memory
    }
    
    // Get memory base address (mask off lower bits)
    e1000_state.mem_base = bar0 & ~0xF;
    
    // Sanity check - make sure we got a valid address
    if (e1000_state.mem_base == 0 || e1000_state.mem_base == 0xFFFFFFF0) {
        return false;
    }
    
    e1000_state.rx_tail = 0;
    e1000_state.tx_tail = 0;
    
    // Reset device
    e1000_write(E1000_CTRL, E1000_CTRL_RST);
    for (volatile int i = 0; i < 100000; i++);
    
    // Set link up
    e1000_write(E1000_CTRL, E1000_CTRL_SLU);
    
    // Read MAC from EEPROM
    uint16_t mac_word = e1000_eeprom_read(0);
    e1000_state.mac[0] = mac_word & 0xFF;
    e1000_state.mac[1] = mac_word >> 8;
    mac_word = e1000_eeprom_read(1);
    e1000_state.mac[2] = mac_word & 0xFF;
    e1000_state.mac[3] = mac_word >> 8;
    mac_word = e1000_eeprom_read(2);
    e1000_state.mac[4] = mac_word & 0xFF;
    e1000_state.mac[5] = mac_word >> 8;
    
    // Initialize RX descriptors
    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        e1000_state.rx_descs[i].addr_low = (uint32_t)e1000_state.rx_buffers[i];
        e1000_state.rx_descs[i].addr_high = 0;
        e1000_state.rx_descs[i].status = 0;
    }
    
    // Initialize TX descriptors
    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        e1000_state.tx_descs[i].addr_low = (uint32_t)e1000_state.tx_buffers[i];
        e1000_state.tx_descs[i].addr_high = 0;
        e1000_state.tx_descs[i].cmd = 0;
        e1000_state.tx_descs[i].status = 1; // DD bit
    }
    
    // Setup RX ring
    e1000_write(E1000_RDBAL, (uint32_t)e1000_state.rx_descs);
    e1000_write(E1000_RDBAH, 0);
    e1000_write(E1000_RDLEN, E1000_NUM_RX_DESC * 16);
    e1000_write(E1000_RDH, 0);
    e1000_write(E1000_RDT, E1000_NUM_RX_DESC - 1);
    
    // Setup TX ring
    e1000_write(E1000_TDBAL, (uint32_t)e1000_state.tx_descs);
    e1000_write(E1000_TDBAH, 0);
    e1000_write(E1000_TDLEN, E1000_NUM_TX_DESC * 16);
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);
    
    // Enable RX
    e1000_write(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM);
    
    // Enable TX
    e1000_write(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP);
    
    e1000_state.initialized = true;
    return true;
}

// Send packet
static void e1000_send(const void* data, uint16_t length) {
    if (!e1000_state.initialized || length > 2048) return;
    
    uint16_t tail = e1000_state.tx_tail;
    struct e1000_tx_desc* desc = &e1000_state.tx_descs[tail];
    
    // Wait for descriptor to be available
    while (!(desc->status & 1));
    
    // Copy data
    const uint8_t* src = (const uint8_t*)data;
    for (uint16_t i = 0; i < length; i++) {
        e1000_state.tx_buffers[tail][i] = src[i];
    }
    
    // Setup descriptor
    desc->length = length;
    desc->cmd = 0x0B; // EOP | IFCS | RS
    desc->status = 0;
    
    // Update tail
    e1000_state.tx_tail = (tail + 1) % E1000_NUM_TX_DESC;
    e1000_write(E1000_TDT, e1000_state.tx_tail);
}

// Poll for received packets
static void e1000_poll(void) {
    if (!e1000_state.initialized) return;
    
    uint16_t tail = e1000_state.rx_tail;
    struct e1000_rx_desc* desc = &e1000_state.rx_descs[tail];
    
    while (desc->status & 1) { // DD bit
        // Process packet
        net_driver_rx_packet((const uint8_t*)e1000_state.rx_buffers[tail], desc->length);
        
        // Reset descriptor
        desc->status = 0;
        
        // Move to next
        tail = (tail + 1) % E1000_NUM_RX_DESC;
        e1000_write(E1000_RDT, tail);
        e1000_state.rx_tail = tail;
        desc = &e1000_state.rx_descs[tail];
    }
}

// Get MAC address
static void e1000_get_mac(uint8_t* mac) {
    for (int i = 0; i < ETH_ALEN; i++) {
        mac[i] = e1000_state.mac[i];
    }
}

// Get link status
static bool e1000_get_link_status(void) {
    if (!e1000_state.initialized) return false;
    uint32_t status = e1000_read(E1000_STATUS);
    return (status & 0x02) != 0; // Link up bit
}

// Supported devices
static const DriverID e1000_devices[] = {
    { E1000_VENDOR_ID, E1000_DEV_82540EM, "Intel 82540EM" },
    { E1000_VENDOR_ID, E1000_DEV_82545EM, "Intel 82545EM" },
    { E1000_VENDOR_ID, E1000_DEV_82574L, "Intel 82574L" },
    { E1000_VENDOR_ID, E1000_DEV_82543GC, "Intel 82543GC" },
    { E1000_VENDOR_ID, E1000_DEV_82544EI, "Intel 82544EI" },
    { E1000_VENDOR_ID, E1000_DEV_82544GC, "Intel 82544GC" },
    { E1000_VENDOR_ID, E1000_DEV_82541PI, "Intel 82541PI" },
    { E1000_VENDOR_ID, E1000_DEV_82547GI, "Intel 82547GI" },
    { E1000_VENDOR_ID, E1000_DEV_82571EB, "Intel 82571EB" },
    { E1000_VENDOR_ID, E1000_DEV_82572EI, "Intel 82572EI" },
    { E1000_VENDOR_ID, E1000_DEV_82573E, "Intel 82573E" },
    { E1000_VENDOR_ID, E1000_DEV_82573L, "Intel 82573L" }
};

// Driver interface
const NetworkDriver e1000_driver = {
    .name = "e1000",
    .init = e1000_init,
    .send = e1000_send,
    .poll = e1000_poll,
    .get_mac = e1000_get_mac,
    .get_link_status = e1000_get_link_status,
    .supported_devices = e1000_devices,
    .num_devices = 13
};
