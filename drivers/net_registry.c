/*
 * Network Driver Registry for slopOS
 * 
 * Central registry of all available network drivers
 */

#include "net_driver.h"

// External driver declarations
extern const NetworkDriver rtl8139_driver;
extern const NetworkDriver e1000_driver;

// Registry of all available drivers
const NetworkDriver* network_drivers[] = {
    &rtl8139_driver,
    &e1000_driver,
};

int num_network_drivers = sizeof(network_drivers) / sizeof(network_drivers[0]);
