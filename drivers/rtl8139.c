/*
 * RTL8139 Fast Ethernet Driver for slopOS
 * Realtek RTL8139 (10/100 Mbps)
 */

#include "net_driver.h"

// RTL8139 PCI IDs
#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

// RTL8139 Registers
#define RTL8139_IDR0 0x00      // MAC address
#define RTL8139_MAR0 0x08      // Multicast filter
#define RTL8139_TXSTATUS0 0x10 // Transmit status (4 descriptors)
#define RTL8139_TXADDR0 0x20   // Transmit address (4 descriptors)
#define RTL8139_RBSTART 0x30   // Receive buffer start
#define RTL8139_CR 0x37        // Command register
#define RTL8139_CAPR 0x38      // Current address of packet read
#define RTL8139_IMR 0x3C       // Interrupt mask
#define RTL8139_ISR 0x3E       // Interrupt status
#define RTL8139_RCR 0x44       // Receive config
#define RTL8139_CONFIG1 0x52   // Config register

// Buffer sizes
#define RX_BUFFER_SIZE 8192 + 16
#define TX_BUFFER_SIZE 1536

// I/O port operations
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

// Driver state
static struct {
    uint16_t io_base;
    uint8_t mac[ETH_ALEN];
    uint8_t rx_buffer[RX_BUFFER_SIZE] __attribute__((aligned(4)));
    uint8_t tx_buffer[TX_BUFFER_SIZE] __attribute__((aligned(4)));
    uint16_t rx_index;
    uint8_t tx_current;
    bool initialized;
} rtl8139_state;

// Forward declaration
static void rtl8139_handle_rx(void);

// Initialize RTL8139
static bool rtl8139_init(uint16_t io_base, uint8_t bus, uint8_t slot) {
    rtl8139_state.io_base = io_base;
    rtl8139_state.tx_current = 0;
    rtl8139_state.rx_index = 0;
    
    uint16_t io = io_base;
    
    // Power on
    outb(io + RTL8139_CONFIG1, 0x00);
    
    // Software reset
    outb(io + RTL8139_CR, 0x10);
    while (inb(io + RTL8139_CR) & 0x10);
    
    // Init receive buffer
    outl(io + RTL8139_RBSTART, (uint32_t)rtl8139_state.rx_buffer);
    
    // Enable transmit and receive
    outb(io + RTL8139_CR, 0x0C);
    
    // Configure receive: accept broadcast, multicast, physical match
    outl(io + RTL8139_RCR, 0x0F);
    
    // Read MAC address
    for (int i = 0; i < ETH_ALEN; i++) {
        rtl8139_state.mac[i] = inb(io + RTL8139_IDR0 + i);
    }
    
    rtl8139_state.initialized = true;
    return true;
}

// Send packet
static void rtl8139_send(const void* data, uint16_t length) {
    if (!rtl8139_state.initialized || length > TX_BUFFER_SIZE) {
        return;
    }
    
    // Copy to TX buffer
    const uint8_t* src = (const uint8_t*)data;
    for (uint16_t i = 0; i < length; i++) {
        rtl8139_state.tx_buffer[i] = src[i];
    }
    
    uint16_t io = rtl8139_state.io_base;
    uint8_t tx = rtl8139_state.tx_current;
    
    // Use rotating TX descriptors (0-3)
    outl(io + RTL8139_TXADDR0 + (tx * 4), (uint32_t)rtl8139_state.tx_buffer);
    outl(io + RTL8139_TXSTATUS0 + (tx * 4), length);
    
    rtl8139_state.tx_current = (tx + 1) & 0x03;
}

// Poll for received packets
static void rtl8139_poll(void) {
    if (!rtl8139_state.initialized) return;
    
    uint16_t io = rtl8139_state.io_base;
    
    // Check if receive buffer is not empty
    uint8_t cmd = inb(io + RTL8139_CR);
    if (cmd & 0x01) return; // Buffer empty
    
    rtl8139_handle_rx();
}

// Handle received packet
static void rtl8139_handle_rx(void) {
    uint16_t io = rtl8139_state.io_base;
    uint16_t capr = rtl8139_state.rx_index;
    uint8_t* rx_buf = rtl8139_state.rx_buffer;
    
    // Read packet header (status + length)
    uint16_t status = *(uint16_t*)(rx_buf + capr);
    uint16_t len = *(uint16_t*)(rx_buf + capr + 2);
    
    // Validate packet
    if ((status & 0x01) && len > 0 && len < ETH_FRAME_LEN) {
        // Call the network stack with received packet
        net_driver_rx_packet((const uint8_t*)(rx_buf + capr + 4), len - 4);
    }
    
    // Update read pointer
    capr = (capr + len + 4 + 3) & ~3; // Align to 4 bytes
    if (capr >= RX_BUFFER_SIZE) capr -= RX_BUFFER_SIZE;
    rtl8139_state.rx_index = capr;
    
    outb(io + RTL8139_CAPR, capr - 16);
}

// Get MAC address
static void rtl8139_get_mac(uint8_t* mac) {
    for (int i = 0; i < ETH_ALEN; i++) {
        mac[i] = rtl8139_state.mac[i];
    }
}

// Get link status
static bool rtl8139_get_link_status(void) {
    // RTL8139 doesn't have easy link status check
    // Assume link is up if initialized
    return rtl8139_state.initialized;
}

// Supported devices
static const DriverID rtl8139_devices[] = {
    { RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, "RTL8139 Fast Ethernet" }
};

// Driver interface
const NetworkDriver rtl8139_driver = {
    .name = "RTL8139",
    .init = rtl8139_init,
    .send = rtl8139_send,
    .poll = rtl8139_poll,
    .get_mac = rtl8139_get_mac,
    .get_link_status = rtl8139_get_link_status,
    .supported_devices = rtl8139_devices,
    .num_devices = 1
};
