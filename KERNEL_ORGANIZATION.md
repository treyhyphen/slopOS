# Kernel Code Organization

## File Structure

The kernel code is organized into the following major sections:

### kernel/kernel.c (Main Kernel - ~3650 lines)

**I/O and Hardware** (Lines 360-730)
- I/O port functions (inb, outb, etc.)
- TSC timing and calibration
- VGA terminal functions
- Keyboard input handling  
- Command history

**Networking Stack** (Lines 960-2450)
- DHCP Client Protocol
- ARP (Address Resolution Protocol)
- IP/UDP packet handling
- ICMP (Ping)
- DNS Client with caching
- Ephemeral port management

**Commands** (Lines 2450-3350)
- **Help**: cmd_help()
- **Filesystem**: cmd_ls(), cmd_mkdir(), cmd_touch(), cmd_cd(), cmd_pwd()
- **User Management**: cmd_whoami(), cmd_listusers(), cmd_passwd(), cmd_adduser(), cmd_deluser()
- **Network**: cmd_ifconfig(), cmd_arp(), cmd_netstat(), cmd_route(), cmd_ping(), cmd_nslookup(), cmd_dhcp()
- **GUI**: cmd_startgui(), cmd_exitgui(), cmd_newwin(), cmd_closewin(), cmd_focuswin(), cmd_listwin(), cmd_writewin()

**Main Loop** (Lines 3350-3650)
- Command parsing
- Kernel initialization
- Main event loop

### kernel/net_integration.c
- Network driver abstraction layer
- Driver registry management
- Packet RX/TX interface

### drivers/rtl8139.c
- RTL8139 Fast Ethernet driver
- PCI device detection
- Packet transmission/reception

### drivers/e1000.c  
- Intel E1000 Gigabit Ethernet driver
- PCI device detection
- Packet transmission/reception

### drivers/net_registry.c
- Network driver registration system
- Multi-driver support

## Future Refactoring Considerations

To split commands into separate files, we would need to:

1. Extract all network-related structures (NetworkDevice, PingState, etc.) into header files
2. Create proper APIs for kernel functions (terminal output, network stack, filesystem)
3. Build a proper command registration system
4. Update Makefile to compile multiple command source files

For now, the code is well-organized with section markers making it easy to navigate.
