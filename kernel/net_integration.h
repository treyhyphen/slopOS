/*
 * Network Driver Integration Header
 */

#ifndef NET_INTEGRATION_H
#define NET_INTEGRATION_H

#include "types.h"

// Initialize network drivers (scans PCI bus and activates first supported NIC)
bool net_driver_init(void);

// Get name of active driver
const char* net_driver_get_name(void);

// Send packet via active driver
void net_driver_send_packet(const void* data, uint16_t length);

// Poll for received packets
void net_driver_poll_packets(void);

// Get MAC address from active driver
void net_driver_get_mac_address(uint8_t* mac);

// Check link status
bool net_driver_link_up(void);

// Check if driver is initialized
bool net_driver_is_initialized(void);

#endif // NET_INTEGRATION_H
