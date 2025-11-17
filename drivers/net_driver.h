/*
 * Network Driver Interface for slopOS
 * 
 * This header defines the common interface that all network drivers must implement.
 * Each driver provides initialization, packet transmission, and packet reception functions.
 */

#ifndef NET_DRIVER_H
#define NET_DRIVER_H

#include "../kernel/types.h"

// Driver identification
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    const char* name;
} DriverID;

// Network driver interface
typedef struct NetworkDriver {
    const char* name;
    
    // Initialize the network card
    // Returns: true on success, false on failure
    bool (*init)(uint16_t io_base, uint8_t bus, uint8_t slot);
    
    // Send a packet
    // data: pointer to packet data
    // length: packet length in bytes
    void (*send)(const void* data, uint16_t length);
    
    // Poll for received packets
    // Calls the receive callback for each packet received
    void (*poll)(void);
    
    // Get MAC address
    // mac: pointer to 6-byte array to fill with MAC address
    void (*get_mac)(uint8_t* mac);
    
    // Get link status
    // Returns: true if link is up, false if down
    bool (*get_link_status)(void);
    
    // List of supported device IDs (vendor:device pairs)
    const DriverID* supported_devices;
    int num_devices;
} NetworkDriver;

// Driver registry
extern const NetworkDriver* network_drivers[];
extern int num_network_drivers;

// This function must be implemented by the kernel
// Drivers call this when they receive a packet
void net_driver_rx_packet(const uint8_t* data, uint16_t length);

#endif // NET_DRIVER_H
