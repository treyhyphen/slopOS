# Building slopOS with Networking and DHCP

## Important: Use the Correct Makefile!

There are multiple Makefiles in the project:

- **`Makefile`** - Builds the simulator (not the bootable kernel)
- **`Makefile.simple`** - Builds from `src/slopos.c` (old version)
- **`Makefile`** - Builds from `kernel/kernel.c` ✅ **USE THIS ONE!**

## Quick Build

```bash
# Clean previous builds
make clean

# Build kernel binary
make all

# Create bootable ISO
make iso
```

## All-in-One Command

```bash
make clean all iso
```

## Testing with DHCP

### Option 1: Use test script
```bash
./test-dhcp.sh
```

### Option 2: Manual QEMU
```bash
qemu-system-i386 \
    -cdrom slopos.iso \
    -m 32M \
    -netdev user,id=net0 \
    -device rtl8139,netdev=net0 \
    -nographic
```

### Option 3: With graphics
```bash
qemu-system-i386 \
    -cdrom slopos.iso \
    -m 32M \
    -netdev user,id=net0 \
    -device rtl8139,netdev=net0
```

## What to Expect

### Boot Messages
```
Initializing system...
  [OK] VGA display initialized
  [OK] Keyboard driver loaded
  [OK] Filesystem initialized
  [OK] Authentication system ready
  [OK] Network card detected (RTL8139)
  [..] Requesting IP via DHCP...
  DHCP: Sending DISCOVER...
  [OK] DHCP configuration successful
       IP: 10.0.2.15
```

### After Login
```bash
admin@slopOS:/$ ifconfig
eth0: RTL8139 Fast Ethernet
  [DHCP configured]         # <-- Look for this!
  HWaddr: 52:54:00:12:34:56
  inet addr: 10.0.2.15      # <-- Assigned by DHCP
  Gateway: 10.0.2.2
  Netmask: 255.255.255.0
  DNS: 10.0.2.3             # <-- New!

admin@slopOS:/$ netstat
Network statistics:
  RX packets: 12            # Should be > 0 if DHCP worked
  RX errors:  0
  TX packets: 4
  TX errors:  0
```

## Troubleshooting

### "Network device not initialized"
**Problem**: RTL8139 not detected
**Solution**: Make sure you have `-device rtl8139,netdev=net0` in QEMU command

### "[WARN] DHCP failed, no IP assigned"
**Problem**: No DHCP response
**Solution**: 
- Check that `-netdev user,id=net0` is in QEMU command
- QEMU user networking includes a DHCP server by default
- Try running `dhcp` command manually after login

### "[No IP address]" in ifconfig
**Problem**: DHCP didn't complete
**Solution**:
- Run `dhcp` command to retry
- Check `netstat` - RX packets should increase if network is working

### Build uses old code
**Problem**: Using wrong Makefile
**Solution**: Always use `make`

### Kernel size is ~32KB instead of ~45KB
**Problem**: Built without DHCP code
**Solution**: 
```bash
make clean all iso
```

## File Locations

### Source Files
- **kernel/kernel.c** - Main kernel with networking and DHCP ✅
- **boot/boot.s** - Boot loader assembly
- **kernel/linker.ld** - Linker script

### Build Outputs
- **kernel/kernel.o** - Compiled kernel (~36KB with DHCP)
- **slopos.bin** - Kernel binary (~41KB with DHCP)
- **slopos.iso** - Bootable ISO (~5MB)

### Build Directory
- **isodir/** - ISO filesystem structure

## Verifying DHCP is Included

### Check kernel size
```bash
ls -lh slopos.bin
# Should show ~41KB (with DHCP)
# Old version was ~32KB (without DHCP)
```

### Check for DHCP symbols
```bash
nm slopos.bin | grep -i dhcp
# Should show dhcp functions if included
```

### Boot and look for messages
During boot, you should see:
- `[..] Requesting IP via DHCP...`
- `DHCP: Sending DISCOVER...`

## Common Mistakes

❌ **Using `make -f Makefile.simple`**
- Builds from `src/slopos.c` (old version without DHCP)

❌ **Using just `make`**
- Builds simulator, not bootable kernel

❌ **Forgetting to rebuild after editing kernel.c**
- Old binary won't have your changes

## ✅ Recommended Workflow

✅ **Always use: `make clean all iso`**

### Quick Test Build

```bash
make clean all iso

## Quick Reference

```bash
# Build everything from scratch
make clean all iso

# Test with DHCP
./test-dhcp.sh

# Or manual test
qemu-system-i386 -cdrom slopos.iso -m 32M \
    -netdev user,id=net0 -device rtl8139,netdev=net0

# Check kernel size (should be ~41KB)
ls -lh slopos.bin

# Exit QEMU (in -nographic mode)
# Press: Ctrl+A, then X
```

## Summary

1. ✅ Edit code in `kernel/kernel.c`
2. ✅ Build with `make clean all iso`
3. ✅ Test with `./test-dhcp.sh` or manual QEMU command
4. ✅ Look for DHCP messages during boot
5. ✅ Run `ifconfig` to verify [DHCP configured] status

---

**Remember**: Always use `Makefile` to build the bootable kernel with networking!
