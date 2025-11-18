# Multi-Driver Network Architecture

## Overview

slopOS now uses a modular, extensible network driver architecture that supports multiple network interface cards through a common abstraction layer.

## Architecture

### Components

```
┌─────────────────────────────────────────┐
│          kernel/kernel.c                │
│   (Network protocol stack: ARP, IP,     │
│    ICMP, UDP, DHCP)                     │
└──────────────┬──────────────────────────┘
               │
               ↓
┌─────────────────────────────────────────┐
│    kernel/net_integration.c             │
│   - PCI bus scanning                    │
│   - Driver selection and init           │
│   - Packet send/receive interface       │
└──────────────┬──────────────────────────┘
               │
               ↓
┌─────────────────────────────────────────┐
│    drivers/net_driver.h                 │
│   (NetworkDriver interface)             │
└──────────────┬──────────────────────────┘
               │
       ┌───────┴───────┬─────────────┐
       ↓               ↓             ↓
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│ RTL8139     │ │ e1000       │ │ Future      │
│ driver      │ │ driver      │ │ drivers...  │
│ (active)    │ │ (disabled)  │ │             │
└─────────────┘ └─────────────┘ └─────────────┘
```

### Files

- **`drivers/net_driver.h`**: Common interface definition for all network drivers
- **`drivers/net_registry.c`**: Central registry of available drivers
- **`drivers/rtl8139.c`**: Realtek RTL8139 Fast Ethernet driver
- **`drivers/e1000.c`**: Intel e1000 Gigabit Ethernet driver (disabled)
- **`kernel/net_integration.c`**: Bridge between kernel and driver layer
- **`kernel/net_integration.h`**: Integration layer API
- **`kernel/types.h`**: Common type definitions

## NetworkDriver Interface

All network drivers must implement this interface:

```c
typedef struct NetworkDriver {
    const char* name;                           // Driver name
    bool (*init)(uint16_t io_base, uint8_t bus, uint8_t slot);
    void (*send)(const void* data, uint16_t length);
    void (*poll)(void);                         // Check for received packets
    void (*get_mac)(uint8_t* mac);             // Get MAC address
    bool (*get_link_status)(void);             // Check link up/down
    const DriverID* supported_devices;         // Array of vendor:device IDs
    int num_devices;                           // Number of supported devices
} NetworkDriver;
```

## Initialization Flow

1. **Boot**: `kernel_main()` calls `net_driver_init()`
2. **PCI Scan**: `net_integration.c` scans PCI bus (buses 0-7, slots 0-31)
3. **Device Match**: For each device found, check against all registered drivers
4. **Driver Init**: When a match is found, call `driver->init(io_base, bus, slot)`
5. **MAC Address**: Retrieve MAC address from driver
6. **DHCP**: If successful, automatically run DHCP client

## Packet Flow

### Transmit (TX)
```
kernel/kernel.c: net_send()
    ↓
kernel/net_integration.c: net_driver_send_packet()
    ↓
drivers/rtl8139.c: rtl8139_send()
    ↓
Hardware
```

### Receive (RX)
```
Hardware
    ↓
drivers/rtl8139.c: rtl8139_poll() → net_driver_rx_packet()
    ↓
kernel/net_integration.c (returns to kernel)
    ↓
kernel/kernel.c: net_poll() → process_packet()
```

## Supported Drivers

### RTL8139 (Active)
- **File**: `drivers/rtl8139.c`
- **Vendor:Device**: 10EC:8139
- **Status**: ✅ Fully functional
- **Features**:
  - Port-mapped I/O
  - 8KB RX buffer
  - 4 rotating TX descriptors
  - Link status detection

### Intel e1000 (Disabled)
- **File**: `drivers/e1000.c`
- **Vendor:Device**: 8086:100E, 8086:100F, 8086:10D3
- **Status**: 🚧 Driver skeleton complete, needs memory-mapped I/O implementation
- **Features**:
  - 16 RX descriptors
  - 8 TX descriptors
  - EEPROM MAC address reading
  - Link status via STATUS register

## Adding a New Driver

### Step 1: Create Driver File

Create `drivers/mydriver.c`:

```c
#include "net_driver.h"

// Define supported devices
static const DriverID mydriver_devices[] = {
    { 0x1234, 0x5678, "My Network Card" }
};

// Implement interface functions
static bool mydriver_init(uint16_t io_base, uint8_t bus, uint8_t slot) {
    // Initialize hardware
    return true;
}

static void mydriver_send(const void* data, uint16_t length) {
    // Transmit packet
}

static void mydriver_poll(void) {
    // Check for received packets
    // Call net_driver_rx_packet() for each packet
}

static void mydriver_get_mac(uint8_t* mac) {
    // Copy MAC address
}

static bool mydriver_get_link_status(void) {
    return true;
}

// Export driver interface
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

### Step 2: Register Driver

Edit `drivers/net_registry.c`:

```c
extern const NetworkDriver mydriver_driver;

const NetworkDriver* network_drivers[] = {
    &rtl8139_driver,
    &mydriver_driver,  // Add here
};
```

### Step 3: Update Makefile

Edit `Makefile`:

```makefile
DRIVER_OBJS = drivers/rtl8139.o drivers/e1000.o drivers/mydriver.o drivers/net_registry.o

drivers/mydriver.o: drivers/mydriver.c
	$(CC) $(CFLAGS) $< -o $@
```

### Step 4: Build and Test

```bash
make clean all iso
qemu-system-i386 -cdrom slopos.iso -m 32M -netdev user,id=net0 -device mydevice,netdev=net0
```

## Benefits

1. **Modularity**: Each driver is self-contained
2. **Extensibility**: Easy to add new hardware support
3. **Maintainability**: Clear separation of concerns
4. **Testability**: Drivers can be individually tested
5. **Code Reuse**: Common code in integration layer

## Future Enhancements

- [ ] Fix e1000 memory-mapped I/O support
- [ ] Add RTL8168 (newer Realtek) support
- [ ] Add virtio-net support for modern QEMU/KVM
- [ ] Implement interrupt-driven RX instead of polling
- [ ] Add MSI/MSI-X interrupt support
- [ ] Create driver hot-swap capability
- [ ] Add network statistics per-driver

## Build Information

- **Kernel size with RTL8139 only**: ~46 KB
- **Compilation**: GCC with `-m32 -ffreestanding`
- **Linking**: Custom linker script `kernel/linker.ld`
- **Dependencies**: None (freestanding environment)

## Testing

Use the provided test script:

```bash
./test-dhcp.sh
```

This will:
1. Boot slopOS ISO
2. Auto-boot after 5 seconds (GRUB timeout)
3. Detect network card via PCI scan
4. Load appropriate driver
5. Automatically run DHCP client
6. Display assigned IP address

## Troubleshooting

### No network card detected
- Check QEMU network device type: `-device rtl8139,netdev=net0`
- Verify PCI vendor:device ID in `drivers/net_registry.c`

### Driver fails to initialize
- Check I/O port permissions (not applicable in QEMU)
- Verify bus mastering is enabled in PCI config

### No DHCP response
- Confirm QEMU user networking: `-netdev user,id=net0`
- Check DHCP server is available (QEMU provides one at 10.0.2.2)

## References

- RTL8139 datasheet: Realtek Semiconductor
- Intel 8254x Gigabit Ethernet Controller datasheets
- PCI Local Bus Specification
- IEEE 802.3 Ethernet standard
