#!/bin/bash
# Quick test for DHCP functionality in slopOS

echo "================================"
echo "slopOS DHCP Test"
echo "================================"
echo ""
echo "This will boot slopOS with networking enabled."
echo "GRUB will auto-boot after 5 seconds."
echo "Watch for DHCP messages during boot:"
echo "  - [..] Requesting IP via DHCP..."
echo "  - DHCP: Sending DISCOVER..."
echo "  - [OK] DHCP configuration successful"
echo "  - IP: 10.0.2.15"
echo ""
echo "After login (admin/slopOS123), try:"
echo "  ifconfig   - Should show [DHCP configured]"
echo "  dhcp       - Manually request DHCP"
echo "  netstat    - View packet statistics"
echo ""
echo "Press Ctrl+A then X to exit QEMU"
echo ""
echo "================================"
sleep 2

qemu-system-i386 \
    -cdrom slopos.iso \
    -m 32M \
    -netdev user,id=net0 \
    -device rtl8139,netdev=net0 \
    -nographic
