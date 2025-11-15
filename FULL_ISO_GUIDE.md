# slopOS Full-Featured ISO Guide

## Overview
This guide explains how to use the full-featured slopOS bootable ISO with authentication, filesystem, and interactive shell.

## Building the ISO

```bash
# Clean and build
make -f Makefile.simple clean
make -f Makefile.simple all
make -f Makefile.simple iso

# Test (automated)
./test-full-iso.sh
```

## Testing Interactively

### Using QEMU
```bash
# Boot the ISO in QEMU with graphical display
qemu-system-i386 -cdrom slopos.iso -m 32

# Alternative: with more RAM and debugging
qemu-system-i386 -cdrom slopos.iso -m 128 -d int,cpu_reset
```

### Using VirtualBox
1. Create new VM: Type "Other", Version "Other/Unknown"
2. RAM: 32MB minimum, 128MB recommended
3. No hard disk needed
4. Settings → Storage → Add optical drive → Select `slopos.iso`
5. Start the VM

### Using VMware
1. Create new VM → I will install OS later
2. Guest OS: Other → Other
3. RAM: 32MB minimum
4. Remove hard disk
5. Settings → CD/DVD → Use ISO image → Browse to `slopos.iso`
6. Power on

## Using slopOS

### Boot Process
1. **GRUB Menu**: Select "slopOS" (or wait 5 seconds for auto-boot)
2. **System Initialization**: Watch initialization messages
3. **Login Prompt**: Enter credentials

### Default Users
```
admin / slopOS123 (administrator)
user  / password  (regular user)
```

**Note**: You have 3 login attempts. Failed attempts will halt the system.

### Available Commands

#### File System Operations
- `ls` - List directory contents (shows files and directories)
- `mkdir <name>` - Create a new directory
- `touch <name>` - Create a new file
- `cd <name>` - Change to directory (use `cd ..` for parent)
- `pwd` - Print current working directory path

#### User Management
- `whoami` - Display current username and role
- `listusers` - List all system users (admin only)

#### System Commands
- `help` - Display available commands
- `clear` - Clear the screen

### Example Session

```
=== slopOS Login ===
Default users:
  admin / slopOS123 (administrator)
  user  / password  (regular user)

Username: admin
Password: **********
Login successful! Welcome, admin.
Administrator privileges granted.

slopOS ready. Type 'help' for available commands.

admin@slopOS:/$ help
Available commands:
  ls             - list directory contents
  mkdir <name>   - create directory
  touch <name>   - create file
  cd <name|..>   - change directory
  pwd            - print working directory
  whoami         - show current user
  listusers      - list all users (admin)
  clear          - clear screen
  help           - show this help

admin@slopOS:/$ mkdir projects
Directory created

admin@slopOS:/$ mkdir documents
Directory created

admin@slopOS:/$ ls
d projects
d documents

admin@slopOS:/$ cd projects
admin@slopOS:/projects$ touch readme.txt
File created

admin@slopOS:/projects$ touch main.c
File created

admin@slopOS:/projects$ ls
- readme.txt
- main.c

admin@slopOS:/projects$ pwd
//projects

admin@slopOS:/projects$ cd ..
admin@slopOS:/$ whoami
admin (administrator)

admin@slopOS:/$ listusers
Active users:
  admin (admin) [current]
  user (user)
```

## Technical Features

### Authentication System
- SHA256-based password hashing (simplified for kernel environment)
- 3-attempt login limit
- User roles (admin/regular)
- Secure password input (masked with asterisks)

### Filesystem
- Hierarchical directory structure
- 256 entry maximum (configurable)
- Parent/child/sibling relationships
- Files and directories distinguished

### Keyboard Driver
- PS/2 keyboard support via I/O ports (0x60/0x64)
- Scancode to ASCII translation
- Backspace support
- Line editing capabilities

### Display
- VGA text mode (80x25 characters)
- 16 foreground colors, 8 background colors
- Screen scrolling
- Colored prompts and messages

### Memory Layout
- Kernel loaded at 1MB (0x00100000)
- Stack below kernel
- VGA buffer at 0xB8000
- 32MB RAM minimum

## Keyboard Layout
- Standard US QWERTY
- Enter: Execute command
- Backspace: Delete character
- Tab: Tab character (not expanded)
- Special keys: Not implemented

## Known Limitations
- No persistence (changes lost on reboot)
- Maximum 256 filesystem entries
- Maximum 16 users
- No file content storage (files are metadata only)
- Single tasking (no multitasking)
- No networking
- No GUI window manager (CLI only)

## Troubleshooting

### Black screen after boot
- Check VM settings: Ensure optical drive is primary boot device
- Verify ISO integrity: `md5sum slopos.iso`

### Keyboard not responding
- Wait 2-3 seconds after boot screen
- Try pressing keys multiple times
- Check VM keyboard input capture

### Login fails immediately
- Ensure correct username (case-sensitive)
- Password: admin=slopOS123, user=password
- 3 attempts maximum before system halt

### Commands not working
- Check spelling (case-sensitive)
- Use `help` to see available commands
- Some commands require arguments: `mkdir name` not `mkdir`

## Development

### Kernel Size
- Minimal kernel: ~15KB
- Full-featured kernel: ~24KB
- ISO image: ~5MB (includes GRUB bootloader)

### Source Files
- `kernel/kernel.c` - Main kernel implementation (~900 lines)
- `boot/boot.s` - Multiboot bootloader
- `kernel/linker.ld` - Memory layout
- `Makefile.simple` - Build system

### Adding Features
Edit `kernel/kernel.c` and rebuild:
1. Add command function: `void cmd_newcommand(const char* arg)`
2. Add to parser: `else if (strcmp(cmd, "newcommand") == 0)`
3. Rebuild: `make -f Makefile.simple clean all iso`
4. Test: `qemu-system-i386 -cdrom slopos.iso -m 32`

## Version Information
- Version: 0.1.0
- Kernel: Full-featured with auth/fs/keyboard
- Updated: 2024
