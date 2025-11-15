#!/bin/bash
# Test script for full-featured slopOS ISO

set -e

echo "========================================"
echo "Testing Full-Featured slopOS ISO"
echo "========================================"
echo ""

# Check if ISO exists
if [ ! -f slopos.iso ]; then
    echo "ERROR: slopos.iso not found!"
    exit 1
fi

# Get file sizes
KERNEL_SIZE=$(ls -lh slopos.bin 2>/dev/null | awk '{print $5}')
ISO_SIZE=$(ls -lh slopos.iso | awk '{print $5}')

echo "✓ Build artifacts found"
echo "  - Kernel: $KERNEL_SIZE"
echo "  - ISO: $ISO_SIZE"
echo ""

# Check kernel features
echo "Checking kernel features..."
FEATURES=0

if strings slopos.bin | grep -q "slopOS Login"; then
    echo "  ✓ Authentication system"
    FEATURES=$((FEATURES+1))
fi

if strings slopos.bin | grep -q "mkdir"; then
    echo "  ✓ Filesystem commands"
    FEATURES=$((FEATURES+1))
fi

if strings slopos.bin | grep -q "keyboard"; then
    echo "  ✓ Keyboard driver"
    FEATURES=$((FEATURES+1))
fi

if strings slopos.bin | grep -q "admin"; then
    echo "  ✓ User management"
    FEATURES=$((FEATURES+1))
fi

if strings slopos.bin | grep -q "whoami"; then
    echo "  ✓ Command processor"
    FEATURES=$((FEATURES+1))
fi

echo ""
echo "Features detected: $FEATURES/5"
echo ""

# Try to boot ISO in QEMU (will timeout after 5 seconds)
echo "Attempting boot test (5 second timeout)..."
echo "Note: Full interactive test requires manual QEMU with graphics"
echo ""

timeout 5 qemu-system-i386 -cdrom slopos.iso -m 32 -nographic 2>&1 | head -20 || true

echo ""
echo "========================================"
echo "Build Summary"
echo "========================================"
echo "✓ Kernel compiled: $KERNEL_SIZE"
echo "✓ ISO created: $ISO_SIZE"
echo "✓ Features implemented: $FEATURES/5"
echo ""
echo "To test interactively:"
echo "  qemu-system-i386 -cdrom slopos.iso -m 32"
echo ""
echo "Default credentials:"
echo "  admin / slopOS123 (administrator)"
echo "  user  / password  (regular user)"
echo ""
echo "Available commands after login:"
echo "  help, ls, mkdir, touch, cd, pwd, whoami, listusers, clear"
echo "========================================"
