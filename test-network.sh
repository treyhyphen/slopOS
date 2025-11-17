#!/bin/bash
# Test script for slopOS networking features

echo "================================"
echo "slopOS Networking Test Suite"
echo "================================"
echo ""

# Build the ISO if needed
if [ ! -f slopos.iso ]; then
    echo "Building slopOS ISO..."
    make -f Makefile.kernel clean all iso
fi

echo "Starting QEMU with networking support..."
echo "Network device: RTL8139 Fast Ethernet"
echo "Network mode: User networking (10.0.2.0/24)"
echo "Guest IP: 10.0.2.15"
echo "Gateway: 10.0.2.2"
echo ""
echo "Available commands in slopOS:"
echo "  - ifconfig    : Show network configuration"
echo "  - arp         : Display ARP cache"
echo "  - netstat     : Show network statistics"
echo "  - ping <ip>   : Send ICMP echo request"
echo ""
echo "You can ping slopOS from another terminal:"
echo "  ping 10.0.2.15"
echo ""
echo "Press Ctrl+A then X to exit QEMU"
echo ""
echo "================================"
echo ""

# Start QEMU with networking
qemu-system-i386 \
    -cdrom slopos.iso \
    -m 32M \
    -netdev user,id=net0 \
    -device rtl8139,netdev=net0 \
    -nographic
