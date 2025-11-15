# slopOS v0.1.0

> *"Why buy Windows when you can have slopOS for free?"* 🪟❌

A hilariously over-engineered hobby operating system that boots on real hardware and does... well, not much. But it does that not-much with **style**! 🎨

Think of it as "My First Operating System" meets "I Have Too Much Free Time". Features include a filesystem that forgets everything when you turn it off, a GUI made of ASCII characters, and enough features to make you wonder "but why though?"

> *"BUT WHY?!"* ❓

idk ... for funsies? Here's the original post I wrote about it:

> I saw a meme of Microsoft bragging about how it uses AI to write 30% of its code now alongside a headline about some recent Windows update that's causing shit to crash. In my head I was like, "Do you want Windows ME again? Because this is how you get Windows ME"
>
> ... but then that got me thinking: How hard would it be to AI slop together my own operating system?
> 
> I know nothing about the C programming language. But I was able to do it in about ~20 minutes, complete with a bootable ISO.
> 
> First iteration just created a "simulator" that was basically a command line executable that would "look" like an operating system when you ran it.
> 
> Second iteration, I had it try to add a window manager GUI, but it really just wound up creating an ASCII text/terminal "GUI". 
> 
> Then I had it add multiuser support, complete with password verification and a very basic permissions structure. This iteration made me laugh because it renamed it to "vibeOS v2.0 - Secure Operating System" haha
> 
> Last iteration was the big one: make this into a real operating system that I could boot with a VM. It figured out it needed a bootloader and a kernel and created both of them. Then it installed QEMU on its own to test it and that actually worked. Last step it figured out it needed to create a bootable ISO. Which it did.
> 
> So I loaded it up in VirtualBox and sure enough, I have a real operating system. In about ~20 minutes of yapping at the robots lol ... this is so ridiculous.

## 🎪 What Makes slopOS Special?

- ✨ **Boots on REAL hardware!** (Your BIOS will be very confused)
- 🔐 **SHA256 password hashing** (because we're securing... nothing)
- 📁 **A filesystem!** (that disappears when you reboot, just like your hopes and dreams)
- 🎨 **ASCII GUI windows!** (it's not a bug, it's retro aesthetic)
- ⌨️ **Full keyboard support!** (Shift key included, unlike certain other projects)
- ⬆️⬇️ **Command history!** (Up/Down arrows work, we're basically professionals now)
- 👥 **Multi-user system!** (admin vs regular user, very serious business)
- 🪟 **Window manager!** (made entirely of `+`, `-`, and `|` characters)

## 🚀 Quick Start (or "How to Waste 5 Minutes")

### Building This Masterpiece

```bash
# Build the simulator (if you're scared of real hardware)
make

# Build the actual bootable ISO (you brave soul)
make -f Makefile.simple clean all iso

# Test in QEMU (safe)
qemu-system-i386 -cdrom slopos.iso -m 32M

# Or burn to USB and pray (DANGEROUS ⚠️)
sudo dd if=slopos.iso of=/dev/sdX bs=4M
# Replace /dev/sdX with your USB. Or don't. I'm not your dad.
```

### Default Credentials
(Yes, we have LOGIN SECURITY for our volatile memory filesystem)

- **Admin**: username `admin`, password `slopOS123` (very secure, much wow)
- **Regular User**: username `user`, password `password` (maximum creativity)

### Your First Commands

```bash
help              # "HELP! What have I done?!"
ls                # Look at all this nothing!
mkdir memes       # Create a directory (it'll be gone in 5 minutes)
cd memes          # Enter the void
touch dank.txt    # Create a "file" (it's just metadata, calm down)
pwd               # "Where am I? Who am I?"
whoami            # Existential crisis simulator
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
startgui                      # Enter the ✨ aesthetic ✨
newwin MyWindow 5 3 30 10     # Create a fancy box
writewin 1 "Hello!"           # Write text in the box
focuswin 1                    # Make it blue (very important)
listwin                       # Admire your creations
closewin 1                    # Destroy everything
exitgui                       # Escape back to reality
```

Windows feature:
- Borders made of `+`, `-`, and `|` (cutting edge graphics)
- Focus indication (blue = focused, because psychology)
- Overlapping windows (they just draw over each other, deal with it)
- Window titles (so you can name your void)

### 👥 User Management
A sophisticated multi-user system for your single-user OS:

```bash
passwd                        # Change your password (why though?)
adduser bob secretpass123     # Create new user (admin only)
deluser bob                   # Destroy Bob (admin only)
listusers                     # See all the users (admin only)
```

*Note: Can't delete yourself. We're not THAT chaotic.*

## 🎮 All Available Commands

| Command | What It Do | Example | Chaos Level |
|---------|-----------|---------|-------------|
| `help` | Shows this list of questionable choices | `help` | ⭐ |
| `ls` | Lists your temporary filesystem entries | `ls` | ⭐ |
| `mkdir` | Makes a directory that'll vanish on reboot | `mkdir temp` | ⭐⭐ |
| `touch` | Creates a file with no content | `touch file.txt` | ⭐⭐ |
| `cd` | Changes directory (supports `..` like a boss) | `cd dir` | ⭐⭐ |
| `pwd` | Shows where you are in the void | `pwd` | ⭐ |
| `whoami` | Existential crisis command | `whoami` | ⭐⭐⭐⭐ |
| `listusers` | Lists all users (admin flex) | `listusers` | ⭐⭐ |
| `passwd` | Change password to something you'll forget | `passwd` | ⭐⭐⭐ |
| `adduser` | Create new user who'll be deleted on reboot | `adduser bob pass` | ⭐⭐⭐⭐ |
| `deluser` | Destroy a user (admin only) | `deluser bob` | ⭐⭐⭐⭐⭐ |
| `startgui` | Enter the ASCII Matrix | `startgui` | ⭐⭐⭐⭐⭐ |
| `newwin` | Create ASCII window | `newwin Title 5 3 30 10` | ⭐⭐⭐⭐ |
| `closewin` | Destroy window | `closewin 1` | ⭐⭐⭐ |
| `focuswin` | Make window blue | `focuswin 1` | ⭐⭐ |
| `listwin` | Admire your ASCII art | `listwin` | ⭐⭐ |
| `writewin` | Put text in window | `writewin 1 "hi"` | ⭐⭐⭐ |
| `exitgui` | Escape the Matrix | `exitgui` | ⭐⭐⭐⭐⭐ |
| `clear` | Make the bad thoughts go away | `clear` | ⭐⭐⭐⭐⭐ |

**Arrow Keys**: ⬆️ = previous command, ⬇️ = next command (stores up to 50, because we're not animals)

## 🏗️ Technical Details (For the Nerds)

- **Architecture**: x86 (32-bit, because 64-bit is too mainstream)
- **Bootloader**: GRUB Multiboot (they did the hard part)
- **Memory**: Loads at 1MB (like it's 1995)
- **Display**: VGA text mode at 0xB8000 (retro!)
- **Keyboard**: PS/2 I/O ports (scancode translation? we got you)
- **Filesystem**: In-memory linked list (256 entries max, we're not Google)
- **Security**: SHA256-simple (we implemented our own, probably broken)
- **Kernel Size**: 32KB (smol bean)
- **ISO Size**: 5MB (most of it is GRUB)

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
- 🌐 **Networking**: What's networking?
- 🖱️ **Mouse**: Use your imagination
- ⌨️ **Keyboard**: US QWERTY only (sorry international friends)
- 🎵 **Sound**: Silence is golden
- 📱 **USB Support**: PS/2 or die
- 🔒 **Security**: We hash passwords for a filesystem that doesn't exist
- 🪟 **GUI**: "Graphics" made of ASCII characters
- 🔄 **Multitasking**: One thing at a time, like my brain

## 🎓 What You'll Learn

- How to waste time productively
- VGA text mode programming (it's 2025, this is peak relevance)
- PS/2 keyboard scancode translation (equally useful)
- Why operating systems are hard
- Why you should appreciate Linux
- How to draw windows with ASCII characters
- That implementing `cd ..` is harder than it looks
- Arrow key command history (actually useful)

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
- [ ] More jokes in this README

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

---

*Made with ☕, ⌨️, and questionable life choices*

**slopOS**: Because "Hello World" was too simple.
