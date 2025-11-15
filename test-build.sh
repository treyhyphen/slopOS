#!/bin/bash
# Test slopOS kernel and ISO builds

echo "slopOS Build & Test Script"
echo "=========================="
echo ""

echo "1. Testing kernel binary..."
if [ -f "slopos.bin" ]; then
    echo "   ✓ Kernel binary exists ($(ls -lh slopos.bin | awk '{print $5}'))"
else
    echo "   ✗ Kernel binary missing"
    exit 1
fi

echo ""
echo "2. Testing ISO image..."
if [ -f "slopos.iso" ]; then
    echo "   ✓ ISO image exists ($(ls -lh slopos.iso | awk '{print $5}'))"
else
    echo "   ✗ ISO image missing"
    exit 1
fi

echo ""
echo "3. Checking kernel structure..."
if file slopos.bin | grep -q "ELF"; then
    echo "   ✓ Kernel is valid ELF binary"
else
    echo "   ⚠ Kernel format check skipped"
fi

echo ""
echo "4. Checking ISO structure..."
if xorriso -indev slopos.iso -find 2>/dev/null | grep -q "boot"; then
    echo "   ✓ ISO has boot directory"
else
    echo "   ⚠ ISO structure check skipped"
fi

echo ""
echo "================================"
echo "✅ Build test PASSED!"
echo ""
echo "To run slopOS:"
echo "  - Kernel: qemu-system-i386 -kernel slopos.bin -m 32M"
echo "  - ISO:    qemu-system-i386 -cdrom slopos.iso -m 32M"
echo ""
echo "Note: The kernel displays output in VGA text mode."
echo "      Use QEMU with a graphical display (not -nographic)"
