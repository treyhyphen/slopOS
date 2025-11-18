# Network Driver Quick Reference

## Building
```bash
make clean all iso
```

## Testing
```bash
./test-dhcp.sh
# or
qemu-system-i386 -cdrom slopos.iso -m 32M -netdev user,id=net0 -device rtl8139,netdev=net0
```

## Architecture

### Core Interface (`drivers/net_driver.h`)
```c
typedef struct NetworkDriver {
    const char* name;
    bool (*init)(uint16_t io_base, uint8_t bus, uint8_t slot);
    void (*send)(const void* data, uint16_t length);
    void (*poll)(void);
    void (*get_mac)(uint8_t* mac);
    bool (*get_link_status)(void);
    const DriverID* supported_devices;
    int num_devices;
} NetworkDriver;
```

### Adding a Driver (3 Steps)

1. **Create** `drivers/mydriver.c`:
```c
#include "net_driver.h"

static const DriverID mydriver_devices[] = {
    { 0xVEND, 0xDEVC, "My Card" }
};

static bool mydriver_init(uint16_t io_base, uint8_t bus, uint8_t slot) {
    // Initialize hardware
    return true;
}

static void mydriver_send(const void* data, uint16_t length) {
    // Send packet
}

static void mydriver_poll(void) {
    // Check for RX, call: net_driver_rx_packet(data, len);
}

static void mydriver_get_mac(uint8_t* mac) {
    // Copy MAC address
}

static bool mydriver_get_link_status(void) {
    return true;
}

const NetworkDriver mydriver_driver = {
    .name = "MyDriver",
    .init = mydriver_init,
    .send = mydriver_send,
    .poll = mydriver_poll,
    .get_mac = mydriver_get_mac,
    .get_link_status = mydriver_get_link_status,
    .supported_devices = mydriver_devices,
    .num_devices = 1
};
```

2. **Register** in `drivers/net_registry.c`:
```c
extern const NetworkDriver mydriver_driver;

const NetworkDriver* network_drivers[] = {
    &rtl8139_driver,
    &mydriver_driver,  // Add here
};
```

3. **Build** - Update `Makefile`:
```makefile
DRIVER_OBJS = drivers/rtl8139.o drivers/mydriver.o drivers/net_registry.o

drivers/mydriver.o: drivers/mydriver.c
	$(CC) $(CFLAGS) $< -o $@
```

## Files

| File | Purpose |
|------|---------|
| `drivers/net_driver.h` | Common interface |
| `drivers/net_registry.c` | Driver registry |
| `drivers/rtl8139.c` | RTL8139 driver (active) |
| `drivers/e1000.c` | e1000 driver (disabled) |
| `kernel/net_integration.c` | PCI scan + init |
| `kernel/kernel.c` | Protocol stack |

## Current Drivers

| Driver | Status | Vendor:Device | File |
|--------|--------|---------------|------|
| RTL8139 | ✅ Active | 10EC:8139 | `drivers/rtl8139.c` |
| e1000 | 🚧 Disabled | 8086:100E/100F/10D3 | `drivers/e1000.c` |

## Network Commands

```bash
ifconfig    # Show IP, MAC, status
dhcp        # Request IP via DHCP
arp         # Show ARP cache
netstat     # Show packet stats
ping        # Send ICMP echo (auto-replies only)
```

## Boot Flow

1. GRUB boots kernel (5s timeout)
2. `kernel_main()` calls `net_driver_init()`
3. PCI bus scan (buses 0-7, slots 0-31)
4. Match vendor:device → driver
5. Call `driver->init()`
6. Get MAC address
7. Run DHCP client
8. Network ready

## Packet Flow

**TX**: `net_send()` → `net_driver_send_packet()` → `driver->send()` → hardware

**RX**: hardware → `driver->poll()` → `net_driver_rx_packet()` → `net_poll()` → `process_packet()`

## Key Functions

### Driver calls kernel
```c
void net_driver_rx_packet(const uint8_t* data, uint16_t length);
```

### Kernel calls driver
```c
bool net_driver_init(void);
void net_driver_send_packet(const void* data, uint16_t length);
void net_driver_poll_packets(void);
void net_driver_get_mac_address(uint8_t* mac);
```

## Documentation

- **`DRIVER_ARCHITECTURE.md`**: Complete architecture guide
- **`MULTI_DRIVER_SUMMARY.md`**: Implementation summary
- **`NETWORKING.md`**: Protocol stack documentation
- **`DHCP_IMPLEMENTATION.md`**: DHCP client details

## Debugging

### No network card found
- Check device type: `-device rtl8139,netdev=net0`
- Verify vendor:device ID in driver's `supported_devices`

### Driver init fails
- Check I/O port base address from PCI BAR0
- Verify bus mastering enabled

### No DHCP
- Confirm QEMU networking: `-netdev user,id=net0`
- QEMU DHCP server is at 10.0.2.2

## Size Info

- RTL8139 driver: ~200 lines
- e1000 driver: ~300 lines
- Integration layer: ~140 lines
- Total kernel: 42 KB (with RTL8139)
- ISO size: 5 MB
