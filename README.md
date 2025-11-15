# slopOS v0.1.0

A full-featured hobby operating system written in C with authentication, filesystem, and interactive shell.

## Features

- 🔐 **Authentication System** - User login with password hashing (SHA256-simple)
- 📁 **Hierarchical Filesystem** - Directories, files, and navigation (cd, mkdir, touch, ls, pwd)
- ⌨️ **PS/2 Keyboard Driver** - Interactive command-line interface with line editing
- 🎨 **VGA Text Mode** - 80x25 colored display with scrolling
- 👥 **Multi-User Support** - Admin and regular user roles with permissions
- 💾 **Bootable ISO** - Full x86 kernel with GRUB bootloader

## Quick Start

### Using Pre-built ISO

1. **Download/Build ISO:**
   ```bash
   make -f Makefile.simple clean all iso
   ```

2. **Test in QEMU:**
   ```bash
   qemu-system-i386 -cdrom slopos.iso -m 32
   ```

3. **Login:**
   - Username: `admin` Password: `slopOS123` (administrator)
   - Username: `user` Password: `password` (regular user)

4. **Try Commands:**
   ```bash
   help              # Show available commands
   mkdir projects    # Create directory
   ls                # List contents
   cd projects       # Change directory
   touch readme.txt  # Create file
   pwd               # Show current path
   whoami            # Show current user
   ```

## Building from Source

### Prerequisites
- GCC (32-bit support)
- NASM (x86 assembler)
- GRUB tools (grub-mkrescue, xorriso, mtools)
- QEMU (for testing)

### Build Commands
```bash
# Build simulator (standard C)
gcc src/main.c -o slopOS

# Build bootable ISO
make -f Makefile.simple clean    # Clean previous build
make -f Makefile.simple all      # Compile kernel
make -f Makefile.simple iso      # Create ISO

# Test
./test-full-iso.sh               # Automated tests
qemu-system-i386 -cdrom slopos.iso -m 32  # Interactive test
```

## Available Commands

| Command | Description | Example |
|---------|-------------|---------|
| `help` | Show all commands | `help` |
| `ls` | List directory contents | `ls` |
| `mkdir <name>` | Create directory | `mkdir projects` |
| `touch <name>` | Create file | `touch readme.txt` |
| `cd <name>` | Change directory | `cd projects` or `cd ..` |
| `pwd` | Print working directory | `pwd` |
| `whoami` | Show current user | `whoami` |
| `listusers` | List all users (admin only) | `listusers` |
| `clear` | Clear screen | `clear` |

## Project Structure

```
slopOS/
├── src/
│   └── main.c              # OS simulator (standard C)
├── kernel/
│   ├── kernel.c            # Bootable kernel (~900 lines)
│   └── linker.ld           # Memory layout
├── boot/
│   └── boot.s              # Multiboot bootloader
├── Makefile.simple         # Build system
├── test-full-iso.sh        # Automated testing
├── FULL_ISO_GUIDE.md       # Detailed usage guide
└── README.md               # This file
```

## Technical Details

- **Architecture:** x86 (32-bit)
- **Bootloader:** GRUB (Multiboot specification)
- **Display:** VGA text mode (0xB8000)
- **Keyboard:** PS/2 via I/O ports (0x60, 0x64)
- **Memory:** Kernel at 1MB, 32MB RAM minimum
- **Filesystem:** In-memory hierarchical (256 entries max)
- **Authentication:** SHA256-simple password hashing

## Documentation

- **[Full ISO Guide](FULL_ISO_GUIDE.md)** - Complete guide for bootable ISO
- **[Version Script](version.sh)** - Version management tool

## Development

### Version Management
```bash
# Set version via script
./version.sh 0.2.0

# Or via git tag
git tag v0.2.0
git push origin v0.2.0

# Or via GitHub UI
# Create release with tag v0.2.0
```

### CI/CD Workflows
- **build-iso.yml** - Builds ISO on kernel changes
- **build-and-release.yml** - Creates GitHub releases on tags
- **auto-tag.yml** - Auto-increments patch version

## Testing

### Automated Tests
```bash
./test-full-iso.sh
```

### Manual Testing
```bash
# QEMU (recommended)
qemu-system-i386 -cdrom slopos.iso -m 32

# VirtualBox
# 1. Create VM: Type=Other, RAM=32MB
# 2. Mount slopos.iso
# 3. Boot

# Real Hardware
# 1. Write ISO to USB: sudo dd if=slopos.iso of=/dev/sdX bs=4M
# 2. Boot from USB
```

## Known Limitations

- No persistence (changes lost on reboot)
- Maximum 256 filesystem entries
- No file content storage
- Single-tasking only
- No networking
- US QWERTY keyboard only

## License

Hobby project - Use at your own risk

## Version

Current version: **v0.1.0**
