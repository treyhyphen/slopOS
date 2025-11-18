# slopOS Networking Documentation

## Overview

slopOS now includes a complete networking stack with support for:
- Ethernet frame handling
- ARP (Address Resolution Protocol)
- IPv4
- ICMP (Internet Control Message Protocol)
- UDP (User Datagram Protocol)
- **DHCP Client (Dynamic Host Configuration Protocol)**

# slopOS Networking Documentation

## Overview

slopOS includes a complete networking stack with modular driver architecture supporting:
- Ethernet frame handling
- ARP (Address Resolution Protocol)
- IPv4
- ICMP (Internet Control Message Protocol)
- UDP (User Datagram Protocol)
- **DHCP Client (Dynamic Host Configuration Protocol)**

## Hardware Support

### Multi-Driver Architecture
The kernel uses a **modular driver architecture** that allows support for multiple network card types:

- **drivers/net_driver.h**: Common interface for all network drivers
- **drivers/net_registry.c**: Central registry of available drivers
- **kernel/net_integration.c**: PCI scanning and driver initialization

### Supported Network Cards

#### RTL8139 Fast Ethernet (Active)
- **Vendor:Device**: 10EC:8139
- **Status**: ✅ Fully functional
- **Location**: `drivers/rtl8139.c`
- Widely supported in virtual machines (QEMU, VirtualBox, VMware)
- Automatically detected via PCI enumeration
- Default for QEMU `-netdev user` mode

#### Intel e1000 Gigabit Ethernet (In Development)
- **Vendor:Device**: 8086:100E, 8086:100F, 8086:10D3
- **Status**: 🚧 Driver created but disabled (needs memory-mapped I/O fixes)
- **Location**: `drivers/e1000.c`
- Supports 82540EM, 82545EM, 82574L chipsets
- Uses descriptor rings for TX/RX

### Adding New Drivers

To add support for a new network card:

1. **Create driver file** in `drivers/` (e.g., `drivers/rtl8168.c`)
2. **Implement the NetworkDriver interface**:
   ```c
   const NetworkDriver my_driver = {
       .name = "MyDriver",
       .init = my_init,           // Initialize card
       .send = my_send,           // Transmit packet
       .poll = my_poll,           // Check for RX packets
       .get_mac = my_get_mac,     // Get MAC address
       .get_link_status = my_get_link_status,
       .supported_devices = my_device_ids,
       .num_devices = 1
   };
   ```
3. **Register in `drivers/net_registry.c`**:
   ```c
   extern const NetworkDriver my_driver;
   const NetworkDriver* network_drivers[] = {
       &rtl8139_driver,
       &my_driver,  // Add here
   };
   ```
4. **Update Makefile** to compile the new driver

The kernel automatically scans the PCI bus and activates the first supported card found.

## Network Configuration

### Automatic Configuration (DHCP)
**NEW!** slopOS now automatically requests an IP address via DHCP at boot time!

The DHCP client:
- Sends DISCOVER message on startup
- Receives OFFER from DHCP server
- Sends REQUEST for offered IP
- Receives ACK with configuration
- Automatically configures IP, netmask, gateway, and DNS

If DHCP fails, you can manually retry with the `dhcp` command.

### Default Settings (If DHCP Fails)
When slopOS boots with a network card present, it uses these defaults:

- **IP Address**: 0.0.0.0 (unconfigured until DHCP succeeds)
- **Netmask**: 0.0.0.0
- **Gateway**: 0.0.0.0

With DHCP (typical QEMU assignment):

- **IP Address**: 10.0.2.15
- **Netmask**: 255.255.255.0
- **Gateway**: 10.0.2.2

These settings match QEMU's default user networking configuration.

## Protocols Implemented

### Ethernet (Layer 2)
- Frame parsing and construction
- MAC address handling
- Protocol type identification (ARP, IP)

### ARP (Address Resolution Protocol)
- ARP request/reply handling
- ARP cache with 32 entries
- Automatic response to ARP requests
- Manual ARP cache inspection via `arp` command

### IPv4 (Layer 3)
- Basic IP packet parsing
- IP header checksum validation
- TTL handling
- Protocol demultiplexing (ICMP)

### ICMP (Internet Control Message Protocol)
- Echo request (ping) detection
- Automatic echo reply generation
- Responds to pings from other systems!

### UDP (User Datagram Protocol)
- Basic UDP packet handling
- UDP checksum calculation (optional)
- Port-based protocol demultiplexing
- Used for DHCP client

### DHCP (Dynamic Host Configuration Protocol)
- **DHCP DISCOVER**: Broadcasts request for IP configuration
- **DHCP OFFER**: Receives offer from server
- **DHCP REQUEST**: Requests the offered configuration
- **DHCP ACK**: Receives acknowledgment and applies configuration
- Automatic retry with timeout
- Extracts IP, netmask, gateway, DNS from DHCP options
- Runs automatically at boot

## Available Commands

### `ifconfig`
Display network interface configuration:
```
ifconfig
```

Output shows:
- Hardware (MAC) address
- IP address
- Gateway
- Netmask
- DNS server
- DHCP configuration status

### `dhcp`
Request IP address via DHCP:
```
dhcp
```

Manually triggers DHCP client to obtain network configuration. Useful if:
- DHCP failed at boot
- Network configuration changed
- Want to renew IP address

### `arp`
Display the ARP cache:
```
arp
```

Shows IP-to-MAC address mappings that have been learned.

### `netstat`
Display network statistics:
```
netstat
```

Shows:
- RX packets (received)
- RX errors
- TX packets (transmitted)
- TX errors

### `ping <ip>`
Send an ARP request to a target IP:
```
ping 10.0.2.2
```

Note: This sends an ARP request rather than full ICMP echo. The kernel will automatically respond to incoming ICMP echo requests.

## Testing in QEMU

### Basic Network Test
```bash
qemu-system-i386 -cdrom slopos.iso -m 32M \
    -netdev user,id=net0 \
    -device rtl8139,netdev=net0
```

### Test with Script
```bash
./test-network.sh
```

### Ping slopOS from Host
In QEMU user networking mode, you cannot directly ping the guest from the host. However:

1. slopOS will respond to ARP requests on its network
2. slopOS automatically replies to ICMP echo requests it receives
3. You can use port forwarding to test connectivity

### Port Forwarding Example
```bash
qemu-system-i386 -cdrom slopos.iso -m 32M \
    -netdev user,id=net0,hostfwd=tcp::5555-:80 \
    -device rtl8139,netdev=net0
```

## Architecture

### Packet Flow (Receive)
1. RTL8139 hardware receives frame into RX buffer
2. `net_poll()` called from main loop
3. `handle_ethernet_frame()` parses Ethernet header
4. Frame dispatched based on EtherType:
   - 0x0806 → `handle_arp()`
   - 0x0800 → `handle_ip()`
5. IP packets dispatched by protocol:
   - ICMP (1) → `handle_icmp()`
   - UDP (17) → `handle_udp()`
6. UDP packets dispatched by port:
   - Port 68 (DHCP client) → `handle_dhcp()`
7. ICMP echo requests automatically get replies
8. DHCP messages update network configuration

### Packet Flow (Transmit)
1. Command or protocol handler builds packet
2. Ethernet header constructed with MACs
3. `net_send()` copies to TX buffer
4. RTL8139 TX descriptor programmed
5. Hardware transmits frame

### PCI Enumeration
On boot, the kernel:
1. Scans PCI bus 0, slots 0-31
2. Reads vendor/device ID pairs
3. Matches RTL8139 (10EC:8139)
4. Reads BAR0 for I/O base address
5. Enables bus mastering

### Driver Initialization
1. Power on the device
2. Software reset
3. Configure receive buffer
4. Enable TX/RX
5. Configure receive mode (accept broadcast/multicast)
6. Read MAC address from registers

## Protocol Details

### ARP Cache
- 32-entry cache stores IP→MAC mappings
- Entries added when ARP packets received
- Used for future transmissions
- View with `arp` command

### IP Checksum
Standard Internet checksum algorithm:
- Sum 16-bit words
- Add carries back into sum
- Return one's complement

### ICMP Echo Reply
When ping request received:
1. Extract source IP and MAC
2. Build reply packet (type 0)
3. Copy ID and sequence number
4. Calculate checksums
5. Transmit back to sender

### DHCP Client Protocol
**4-way handshake** (DORA):
1. **DISCOVER**: Broadcast to find DHCP servers
   - Client→Broadcast (255.255.255.255:67)
   - Transaction ID for matching replies
2. **OFFER**: Server offers IP configuration
   - Server→Client (broadcast reply)
   - Includes offered IP (yiaddr)
   - Options: netmask, gateway, DNS, lease time
3. **REQUEST**: Client accepts offer
   - Client→Broadcast
   - Requests specific IP from specific server
4. **ACK**: Server confirms
   - Server→Client
   - Client applies configuration

**DHCP Message Structure**:
- 236 bytes base message + options
- Transaction ID (XID) for matching
- Client hardware address (chaddr)
- Magic cookie: 0x63825363
- Options: TLV (Type-Length-Value) format

**Key DHCP Options**:
- Option 53: Message Type (DISCOVER=1, OFFER=2, REQUEST=3, ACK=5)
- Option 1: Subnet Mask
- Option 3: Router/Gateway
- Option 6: DNS Server
- Option 50: Requested IP Address
- Option 54: Server Identifier
- Option 55: Parameter Request List
- Option 255: End marker

### UDP Protocol
- **Ports**: 16-bit source and destination
- **Checksum**: Optional for IPv4 (we set to 0)
- **Length**: UDP header + payload
- **Pseudo-header**: Used in checksum calculation

## Limitations

### What's NOT Implemented
- **TCP/UDP**: No transport layer protocols
- **DHCP**: No dynamic IP configuration
- **TCP**: No reliable transport layer
- **DNS**: No domain name resolution
- **Routing**: Single network only
- **Fragmentation**: No IP fragmentation support
- **IPv6**: IPv4 only
- **Multiple interfaces**: Single NIC only
- **Socket API**: No Berkeley sockets
- **DHCP Lease Renewal**: No automatic renewal

### Known Issues
- RX buffer handling is basic (no wraparound protection)
- No interrupt handling (polling only)
- Limited error recovery
- ARP cache has no timeout/invalidation
- No IP forwarding or routing table

## Future Enhancements

Potential additions (in order of feasibility):

1. **DHCP Client**: Auto-configure IP address ✅ **DONE!**
2. **DHCP Lease Renewal**: Auto-renew before expiration
3. **DNS Client**: Resolve domain names
2. **UDP Support**: Simple datagram protocol
3. **DNS Client**: Resolve domain names
4. **TCP Support**: Reliable transport (complex!)
5. **HTTP Client**: Fetch web pages
6. **Better NIC Support**: e1000, virtio-net

## Code Structure

### Network Files
All networking code is in `kernel/kernel.c`:

- **Lines ~30-80**: Network constants and protocol defines (including DHCP)
- **Lines ~90-230**: Network structures (Ethernet, ARP, IP, ICMP, UDP, DHCP headers)
- **Lines ~320-400**: Byte order conversion (htons, ntohs, etc)
- **Lines ~400-500**: PCI enumeration and RTL8139 detection
- **Lines ~500-700**: RTL8139 driver initialization + DHCP client
- **Lines ~700-1100**: DHCP protocol (DISCOVER, REQUEST, parsing)
- **Lines ~1100-1400**: UDP and protocol handlers
- **Lines ~1400-1600**: Network commands (ifconfig, dhcp, arp, netstat, ping)

### Key Data Structures
```c
NetworkDevice net_device;  // Global network state
  - initialized: bool
  - io_base: PCI I/O base address
  - mac[6]: Hardware address
  - ip[4]: IPv4 address
  - gateway[4]: Default gateway
  - netmask[4]: Subnet mask
  - dns[4]: DNS server
  - rx_buffer[]: Receive ring buffer
  - tx_buffer[]: Transmit buffer
  - arp_cache[32]: ARP table
  - dhcp_configured: bool
  - dhcp_xid: Transaction ID
  - dhcp_server_ip[4]: DHCP server
  - statistics: counters
```

## Debugging Tips

### Check if NIC is detected
```
ifconfig
```
If you see "Network device not initialized", the RTL8139 wasn't found.

### Check DHCP status
```
ifconfig
```
Look for [DHCP configured], [Static IP], or [No IP address] status line.

### Manually request DHCP
```
dhcp
```
If automatic DHCP at boot failed, manually trigger it.

### View packet statistics
```
netstat
```
Watch RX/TX packet counters to see if traffic is flowing.

### Inspect ARP learning
```
ping 10.0.2.2
arp
```
Should show the gateway's MAC address learned via ARP.

### QEMU Debug
Add `-d guest_errors` to QEMU command line to see hardware errors.

## Technical References

- **RTL8139**: Realtek 8139C+ datasheet
- **IEEE 802.3**: Ethernet standard
- **RFC 826**: ARP specification
- **RFC 791**: IPv4 specification
- **RFC 792**: ICMP specification
- **RFC 768**: UDP specification
- **RFC 2131**: DHCP specification
- **PCI Local Bus**: PCI configuration space

## Conclusion

slopOS networking is intentionally simple but functional. It demonstrates:
- Real hardware driver development
- Network protocol implementation
- Packet parsing and construction
- State management (ARP cache)
- **DHCP client implementation** (auto-configuration!)
- UDP datagram handling

Perfect for learning bare-metal network programming without the complexity of a full TCP/IP stack!

---

*"We went from 'Hello World' to 'Hello DHCP Server' and got an IP address automatically. Progress!"*
