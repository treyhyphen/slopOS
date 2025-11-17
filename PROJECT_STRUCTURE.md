# slopOS Project Structure

## Active Source Files

### Kernel
- `kernel/kernel.c` - Main kernel (55KB) with:
  - Filesystem, authentication, user management
  - GUI mode with windowing system
  - Network stack (Ethernet, ARP, IPv4, ICMP, UDP, DHCP)
  - Interactive ping, ifconfig, arp, netstat commands
- `kernel/net_integration.c` - PCI bus scanning & driver initialization
- `kernel/net_integration.h` - Network integration API
- `kernel/types.h` - Common type definitions
- `kernel/linker.ld` - Linker script

### Drivers
- `drivers/net_driver.h` - Network driver interface
- `drivers/rtl8139.c` - Realtek RTL8139 Fast Ethernet driver
- `drivers/e1000.c` - Intel PRO/1000 Gigabit Ethernet driver (13 variants)
- `drivers/net_registry.c` - Driver registry

### Boot
- `boot/boot.s` - GRUB multiboot bootloader
- `isodir/boot/grub/grub.cfg` - GRUB configuration

## Build System
- `Makefile.kernel` - Primary build system
- Build: `make -f Makefile.kernel all iso`
- Clean: `make -f Makefile.kernel clean`

## Binaries
- `slopos.bin` - Bootable kernel (55KB)
- `slopos.iso` - Bootable ISO image (5MB)

## Testing Scripts
- `run-vibeos.sh` - Quick QEMU test with RTL8139
- `test-network.sh` - Network functionality tests
- `demo.sh` - Demo script

## Documentation
- `README.md` - Main documentation
- `BUILD_GUIDE.md` - Build instructions
- `DRIVER_ARCHITECTURE.md` - Network driver architecture
- `NETWORKING.md` - Network stack documentation
- `QUICKREF.txt` - Quick reference

## Features
✅ Multi-user authentication (SHA-256)
✅ Virtual filesystem with directories
✅ User management (passwd, adduser, deluser)
✅ GUI mode with windowing system
✅ Multi-driver network support (RTL8139, Intel e1000)
✅ DHCP client with auto-configuration
✅ Interactive ICMP ping with statistics
✅ ARP cache management
✅ Network diagnostics (ifconfig, arp, netstat)
