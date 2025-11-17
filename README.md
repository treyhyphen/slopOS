# slopOS v0.1.0

## WTF WHY

I saw a meme of Microsoft bragging about how it uses AI to write 30% of its code now alongside a headline about some recent Windows update that's causing shit to crash. In my head I was like, _**"Do you want Windows ME again? Because this is how you get Windows ME"**_

... but then that got me thinking: _**How hard would it be to AI slop together my own operating system?**_

I know nothing about the C programming language. But I was able to do it in about ~20 minutes, complete with a bootable ISO.
 
* First iteration just created a "simulator" that was basically a command line executable that would "look" like an operating system when you ran it.
* Second iteration, I had it try to add a window manager GUI, but it really just wound up creating an ASCII text/terminal "GUI". 
* Then I had it add multiuser support, complete with password verification and a very basic permissions structure. This iteration made me laugh because it renamed it to "vibeOS v2.0 - Secure Operating System"
* Last iteration was the big one: make this into a real operating system that I could boot with a VM. It figured out it needed a bootloader and a kernel and created both of them. Then it installed QEMU on its own to test it and that actually worked. Last step it figured out it needed to create a bootable ISO. Which it did.

So I loaded it up in VirtualBox and sure enough, I have a real operating system. In about ~20 minutes of yapping at the robots lol ... this is so ridiculous.

So now it's out here on GitHub! Because "reasons"! Maybe I'll slop some new features in here at some point.

Everything except this specific section of the readme was entirely vibed together with GitHub Copilot w/ Claude Sonnet 4.5. I've tried to keep exporting and preserving my chats in the `.copilot_chats` directory as best as I can remember

## 🎪 What Makes slopOS Special?

- ✨ **Boots on REAL hardware!** (Your BIOS will be very confused)
- 🔐 **SHA256 password hashing** (because we're securing... nothing)
- 📁 **A filesystem!** (that disappears when you reboot, just like your hopes and dreams)
- 🎨 **ASCII GUI windows!** (it's not a bug, it's retro aesthetic)
- ⌨️ **Full keyboard support!** (Shift key included, unlike certain other projects)
- ⬆️⬇️ **Command history!** (Up/Down arrows work, we're basically professionals now)
- 👥 **Multi-user system!** (admin vs regular user, very serious business)
- 🪟 **Window manager!** (made entirely of `+`, `-`, and `|` characters)
- 🌐 **Networking stack!** (Ethernet, IP, ARP, ICMP, UDP, DHCP - full TCP/IP!)
- 🔌 **Modular network drivers!** (RTL8139 + e1000, easily extensible)

## 🚀 Quick Start (or "How to Waste 5 Minutes")

### Building This Masterpiece

**⚠️ Important**: Use `Makefile.kernel` to build the bootable ISO with networking!

```bash
# Build the simulator (command-line version, no networking)
make

# Build the actual bootable ISO with DHCP support ✨
make -f Makefile.kernel clean all iso

# Test in QEMU (basic, no networking)
qemu-system-i386 -cdrom slopos.iso -m 32M

# Test with networking + DHCP (recommended!)
qemu-system-i386 -cdrom slopos.iso -m 32M \
    -netdev user,id=net0 \
    -device rtl8139,netdev=net0

# Or use the test script
./test-dhcp.sh

# Or burn to USB and pray (DANGEROUS ⚠️)
sudo dd if=slopos.iso of=/dev/sdX bs=4M
# Replace /dev/sdX with your USB. Or don't. I'm not your dad.
```

### Default Credentials
(Yes, we have LOGIN SECURITY for our volatile memory filesystem)

- **Admin**: username `admin`, password `slopOS123` (very secure, much wow)
- **Regular User**: username `user`, password `password` (maximum creativity)

### Networking Note
slopOS now includes **DHCP client support**! It will automatically attempt to grab an IP address at boot. If DHCP fails, just run `gimmeip` to retry.

### Your First Commands

```bash
whatdo            # "yo what can i even do here?"
peek              # check out what's vibin in this folder
cook memes        # spawn a new folder (it'll vanish when you reboot tho)
bounce memes      # slide into that folder
yeet dank.txt     # slap down a new "file" (just vibes, no actual data)
whereami          # "wait where am i?"
me                # "who am i even?"
mynet             # peep your network setup
poke 10.0.2.2     # poke the QEMU gateway (if networking is up)
wherez google.com # find where a website lives (DNS lookup)
```

## 📦 Features That Will Make You Question Everything

### 🔐 Authentication System
Because every OS that forgets everything needs password security! Features:
- SHA256 password hashing (overkill? perhaps.)
- 3 failed login attempts and the system just... gives up on life (system halt)
- Admin vs regular user roles (admins can... um... list users. That's it.)

### 📁 "Filesystem" (Use Air Quotes)
A hierarchical filesystem that exists purely in RAM! Features:
- Directories! Files! Metadata!
- Maximum 256 entries (we're not made of memory here)
- `cd ..` actually works (we're very proud of this)
- Stores absolutely zero file content (it's the journey, not the destination)

### ⌨️ PS/2 Keyboard Driver
Lovingly hand-crafted to interpret your keystrokes:
- Full scancode translation (we memorized the PS/2 scancode table so you don't have to)
- Shift key support (IT ACTUALLY WORKS NOW! 🎉)
- Backspace (for when you regret your life choices)
- Arrow keys for command history (⬆️⬇️ like a REAL shell)

### 🎨 VGA Text Mode Display
80x25 characters of pure beauty:
- Hardware cursor that actually follows your typing (revolutionary!)
- Scrolling (when you run out of screen, we just... shift everything up)
- Colors! (well, VGA text mode colors, but still!)
- Clear screen (when the existential dread gets too much)

### 🪟 ASCII GUI "Window Manager"
Yes, we have windows. Made of ASCII art. Because we can.

```bash
govisual                      # Enter the ✨ visual mode ✨
spawn MyWindow 5 3 30 10      # Create a fancy box
scribble 1 "Hello!"           # Write text in the box
focus 1                       # Make it blue (very important)
windows                       # Admire your creations
kill 1                        # Destroy everything
goblind                       # Escape back to text mode
```

Windows feature:
- Borders made of `=` and `|` (cutting edge graphics)
- Focus indication (blue = focused, because psychology)
- Overlapping windows (they just draw over each other, deal with it)
- Window titles (so you can name your void)

### 👥 User Management
A sophisticated multi-user system for your single-user OS:

```bash
passwd                        # Change your password (why though?)
recruit bob secretpass123     # Create new user (admin only)
kickout bob                   # Destroy Bob (admin only)
whosthere                     # See all the vibers (admin only)
```

*Note: Can't delete yourself. We're not THAT chaotic.*

### 🌐 Networking Stack
A real networking implementation for your volatile RAM OS:

```bash
mynet                        # Show network configuration
arp                          # Display ARP cache
netstat                      # Show network statistics
route                        # Show routing table
poke 10.0.2.2                # Send ICMP echo to gateway
gimmeip                      # Request IP via DHCP
wherez google.com            # DNS lookup - find where a site lives
vibe 10.0.2.2 80             # Connect to remote host (netcat-style)
vibe -l 8080                 # Listen for incoming connections
```

Features:
- **RTL8139 + e1000 drivers** (automatically detected via PCI enumeration)
- **Ethernet frame handling** (because we're fancy)
- **ARP protocol** (resolves IP to MAC addresses)
- **IP stack** (basic IPv4 support)
- **TCP protocol** (full connection-oriented communication!)
- **UDP protocol** (for DHCP and DNS)
- **ICMP** (responds to ping requests automatically!)
- **DHCP client** (automatically obtains IP address at boot!)
- **DNS resolver** (look up domain names!)
- **Network statistics** (RX/TX packet counts and errors)
- **Netcat-like tool** (`vibe` command for TCP connections)
- Automatic configuration via DHCP (10.0.2.15/24 default in QEMU)

To enable networking in QEMU:
```bash
qemu-system-i386 -cdrom slopos.iso -m 32M -netdev user,id=net0 -device rtl8139,netdev=net0
```

Now you can ping slopOS from your host! Try: `ping 10.0.2.15`

## 🎮 All Available Commands (Keep It Sloppy)

| Command | What It Do | Example | Chaos Level |
|---------|-----------|---------|-------------|
| `whatdo` | Shows this list of sloppy vibes | `whatdo` | ⭐ |
| `peek` | Peep what's in this folder | `peek` | ⭐ |
| `cook` | Make a folder that'll vanish on reboot | `cook temp` | ⭐⭐ |
| `yeet` | Slap down a file with no content | `yeet file.txt` | ⭐⭐ |
| `bounce` | Jump to another folder (supports `..`) | `bounce dir` | ⭐⭐ |
| `whereami` | Where you at in the filesystem? | `whereami` | ⭐ |
| `me` | Who even are you? | `me` | ⭐⭐⭐⭐ |
| `mynet` | Check your network setup | `mynet` | ⭐⭐ |
| `gimmeip` | Grab an IP via DHCP | `gimmeip` | ⭐⭐⭐ |
| `poke` | Ping a host (default 5 packets) | `poke google.com` | ⭐⭐⭐ |
| `wherez` | DNS lookup for domain | `wherez github.com` | ⭐⭐⭐ |
| `vibe` | Connect/listen TCP (netcat vibes) | `vibe -l 8080` | ⭐⭐⭐⭐⭐ |
| `whosthere` | See all the vibers (admin flex) | `whosthere` | ⭐⭐ |
| `passwd` | Change password to something you'll forget | `passwd` | ⭐⭐⭐ |
| `recruit` | Invite a new user who'll be gone on reboot | `recruit bob pass` | ⭐⭐⭐⭐ |
| `kickout` | Boot a user (admin only) | `kickout bob` | ⭐⭐⭐⭐⭐ |
| `govisual` | Enter visual mode | `govisual` | ⭐⭐⭐⭐⭐ |
| `spawn` | Create ASCII window | `spawn Title 5 3 30 10` | ⭐⭐⭐⭐ |
| `kill` | Destroy window | `kill 1` | ⭐⭐⭐ |
| `focus` | Make window blue (important vibes) | `focus 1` | ⭐⭐ |
| `windows` | Admire your ASCII art | `windows` | ⭐⭐ |
| `scribble` | Put text in window | `scribble 1 "hi"` | ⭐⭐⭐ |
| `goblind` | Exit visual mode | `goblind` | ⭐⭐⭐⭐⭐ |
| `mynet` | Check your net setup | `mynet` | ⭐⭐ |
| `gimmeip` | Auto-grab an IP (DHCP) | `gimmeip` | ⭐⭐⭐ |
| `arp` | See who's on the LAN | `arp` | ⭐⭐ |
| `netstat` | Network stats and vibes | `netstat` | ⭐⭐ |
| `route` | Check the routing table | `route` | ⭐⭐ |
| `poke` | Poke a host with ICMP | `poke 10.0.2.2` | ⭐⭐⭐ |
| `wherez` | Find where a site lives (DNS) | `wherez google.com` | ⭐⭐⭐ |
| `wipe` | Clear the screen (fresh start) | `wipe` | ⭐⭐⭐⭐⭐ |

**Arrow Keys**: ⬆️ = previous command, ⬇️ = next command (stores up to 50, because we're not animals)
**CTRL+C**: Interrupt long-running commands (like `poke`, `gimmeip`, `wherez`)

## 🏗️ Technical Details (For the Nerds)

- **Architecture**: x86 (32-bit, because 64-bit is too mainstream)
- **Bootloader**: GRUB Multiboot (they did the hard part)
- **Memory**: Loads at 1MB (like it's 1995)
- **Display**: VGA text mode at 0xB8000 (retro!)
- **Keyboard**: PS/2 I/O ports (scancode translation? we got you)
- **Filesystem**: In-memory linked list (256 entries max, we're not Google)
- **Security**: SHA256-simple (we implemented our own, probably broken)
- **Networking**: RTL8139 + e1000 drivers with multi-driver support
- **Network Protocols**: Ethernet, ARP, IPv4, UDP, ICMP, DHCP client, DNS resolver
- **Driver Architecture**: Modular network driver system (easily extensible)
- **Kernel Size**: ~50KB (still a smol bean)
- **ISO Size**: 5MB (most of it is GRUB)
- **Command System**: Modular command structure (fs, user, network, gui modules)

## 📁 What's In The Box?

```
slopOS/
├── src/
│   └── slopos.c              # 1500 lines of "why did I do this?"
├── boot/
│   └── boot.s                # Assembly that we copy-pasted
├── kernel/
│   └── linker.ld             # Memory layout (black magic)
├── Makefile                  # Builds the simulator
├── Makefile.simple           # Builds the actual kernel
└── README.md                 # You are here (help)
```

## 🎯 Building From Source (If You Dare)

### What You Need
- GCC with 32-bit support (probably already have it)
- NASM (for the assembly we copy-pasted)
- GRUB tools (grub-mkrescue, xorriso, mtools)
- QEMU (for safe testing before committing hardware crimes)
- A sense of humor (not optional)

### The Build Process

```bash
# Simulator build (safe, boring)
make
./slopOS

# REAL KERNEL BUILD (exciting, dangerous)
make -f Makefile.simple clean
make -f Makefile.simple all
make -f Makefile.simple iso

# Test without risking your actual computer
qemu-system-i386 -cdrom slopos.iso -m 32M

# Test on real hardware (you absolute madlad)
# 1. dd to USB drive
# 2. Boot from USB
# 3. Marvel at what you've done
# 4. Question your life choices
```

## ⚠️ Known "Features" (Definitely Not Bugs)

- 💾 **Persistence**: None. Zero. Nada. Reboot and it's all gone.
- 📊 **File Content**: Files don't actually store data. It's just names. Philosophy!
- 🔢 **Filesystem Size**: 256 entries max. After that? ¯\\\_(ツ)_/¯
- 🖱️ **Mouse**: Use your imagination
- ⌨️ **Keyboard**: US QWERTY only (sorry international friends)
- 🎵 **Sound**: Silence is golden
- 📱 **USB Support**: PS/2 or die
- 🔒 **Security**: We hash passwords for a filesystem that doesn't exist
- 🪟 **GUI**: "Graphics" made of ASCII characters
- 🔄 **Multitasking**: One thing at a time, like my brain
- 🌐 **Networking**: Works with RTL8139 or e1000 in QEMU/VirtualBox
- 📡 **TCP**: What's that? UDP only gang!
- 🔧 **DHCP**: Works great in QEMU user networking mode
- 🌐 **DNS**: Can resolve names but can't browse the web (yet)

## 🎓 What You'll Learn

- How to waste time productively
- VGA text mode programming (it's 2025, this is peak relevance)
- PS/2 keyboard scancode translation (equally useful)
- Why operating systems are hard
- Why you should appreciate Linux
- How to draw windows with ASCII characters
- That implementing `bounce ..` is harder than it looks
- Arrow key command history (actually useful)
- CTRL+C interrupt handling (stop those infinite loops!)
- PCI device enumeration (finding hardware the hard way)
- Network packet parsing (Ethernet frames are fun!)
- Writing network drivers (RTL8139 and e1000)
- Why ARP exists (IP addresses need MAC addresses)
- DHCP protocol (DISCOVER, OFFER, REQUEST, ACK dance)
- DNS resolution (turning names into IPs)
- UDP implementation (simpler than TCP, still useful)
- Modular driver architecture (making code less sloppy)

## 🤝 Contributing

If you actually want to contribute to this... thank you? I guess?

Things that would make slopOS marginally less useless:
- [ ] File content storage (radical idea)
- [ ] Persistent filesystem (even more radical)
- [ ] Mouse support (heresy)
- [ ] More keyboard layouts (international chaos)
- [ ] A better GUI (ASCII art is sacred though)
- [ ] Actual file operations (read/write/edit)
- [ ] Tab completion (fancy!)
- [x] ~~Multi-driver support~~ (WE DID IT!)
- [x] ~~DHCP client~~ (DONE!)
- [x] ~~DNS resolver~~ (DONE!)
- [x] ~~CTRL+C interrupts~~ (DONE!)
- [ ] TCP support (ambitious!)
- [ ] HTTP client (web browsing in ASCII?)
- [ ] More sloppy command names (never enough slop)

## 📜 License

Do whatever you want with this. If you somehow make money from it, please tell me how.

## 🙏 Acknowledgments

- The entire Internet for StackOverflow answers
- GRUB developers for doing the hard boot stuff
- PS/2 keyboard for still existing in VirtualBox
- Anyone who actually boots this on real hardware (you're braver than me)
- My CPU for not catching fire during development

## 📫 Support

If something breaks:
1. That's normal
2. Did you reboot? Everything's gone anyway
3. Check that you typed the command right
4. Try turning it off and on again (literally)
5. Remember: it's a hobby OS, expectations should be LOW

## 🎪 Current Version

**v0.1.0** - "It Boots, I Guess" Edition

Features:
- Boots (sometimes)
- Accepts input (usually)
- Displays output (mostly)
- Doesn't catch fire (yet)
- Responds to pings (if you're lucky)
- Gets its own IP via DHCP (automatically!)

---

*Made with ☕, ⌨️, and questionable life choices*

**slopOS**: Because "Hello World" was too simple.
