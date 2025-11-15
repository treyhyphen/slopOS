/*
 * slopOS v0.1.0 - Unified Source
 * Compiles as either:
 *   - Simulator (standard C with stdio)
 *   - Bootable Kernel (freestanding C with hardware access)
 * 
 * Build modes:
 *   gcc src/slopos.c -o slopOS                    # Simulator
 *   gcc -DKERNEL_MODE -m32 -c src/slopos.c ...    # Kernel
 */

// ====================
// MODE DETECTION & HEADERS
// ====================

#ifdef KERNEL_MODE
    // Freestanding kernel mode - define our own types
    typedef unsigned char uint8_t;
    typedef unsigned short uint16_t;
    typedef unsigned int uint32_t;
    typedef unsigned long size_t;
    typedef int bool;
    #define true 1
    #define false 0
    #define NULL ((void*)0)
#else
    // Simulator mode - use standard library
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <unistd.h>
    #include <ctype.h>
    #include <termios.h>
#endif

// ====================
// CONFIGURATION CONSTANTS
// ====================

#define MAX_NAME 64
#ifdef KERNEL_MODE
    #define MAX_ENTRIES 256  // Limited in kernel
#else
    #define MAX_ENTRIES 1024 // More in simulator
#endif
#define MAX_USERS 16
#define MAX_PASSWORD 32
#define SHA256_DIGEST_LENGTH 32

#ifdef KERNEL_MODE
    #define MAX_WINDOWS 8
    #define SCREEN_HEIGHT 25
#else
    #define MAX_WINDOWS 16
    #define SCREEN_HEIGHT 24
#endif
#define SCREEN_WIDTH 80

// ====================
// KERNEL-SPECIFIC DEFINES
// ====================

#ifdef KERNEL_MODE
    #define VGA_MEMORY 0xB8000
    #define VGA_WIDTH 80
    #define VGA_HEIGHT 25
    #define KEYBOARD_DATA_PORT 0x60
    #define KEYBOARD_STATUS_PORT 0x64
    #define KEY_LSHIFT 0x2A
    #define KEY_RSHIFT 0x36
#endif

// ====================
// COLOR DEFINITIONS
// ====================

#ifdef KERNEL_MODE
    enum vga_color {
        VGA_COLOR_BLACK = 0,
        VGA_COLOR_BLUE = 1,
        VGA_COLOR_GREEN = 2,
        VGA_COLOR_CYAN = 3,
        VGA_COLOR_RED = 4,
        VGA_COLOR_MAGENTA = 5,
        VGA_COLOR_BROWN = 6,
        VGA_COLOR_LIGHT_GREY = 7,
        VGA_COLOR_DARK_GREY = 8,
        VGA_COLOR_LIGHT_BLUE = 9,
        VGA_COLOR_LIGHT_GREEN = 10,
        VGA_COLOR_LIGHT_CYAN = 11,
        VGA_COLOR_LIGHT_RED = 12,
        VGA_COLOR_PINK = 13,
        VGA_COLOR_YELLOW = 14,
        VGA_COLOR_WHITE = 15,
    };
#else
    // ANSI color codes for simulator
    #define COLOR_RESET "\033[0m"
    #define COLOR_CYAN "\033[1;36m"
    #define COLOR_GREEN "\033[1;32m"
    #define COLOR_RED "\033[1;31m"
    #define COLOR_YELLOW "\033[1;33m"
    #define COLOR_BLUE "\033[1;34m"
    #define COLOR_MAGENTA "\033[1;35m"
#endif

// ====================
// DATA STRUCTURES
// ====================

typedef enum { FILE_ENTRY, DIR_ENTRY } EntryType;

typedef struct Entry {
    char name[MAX_NAME];
    EntryType type;
    int parent;
    int first_child;
    int next_sibling;
} Entry;

typedef struct User {
    char username[MAX_NAME];
    uint8_t password_hash[SHA256_DIGEST_LENGTH];
    bool is_admin;
    bool active;
} User;

typedef struct AuthSystem {
    User users[MAX_USERS];
    int user_count;
    int current_user;
    bool logged_in;
} AuthSystem;

typedef struct Window {
    int id;
    char title[MAX_NAME];
    int x, y;
    int width, height;
    bool visible;
    bool focused;
    char buffer[1024];
    int buffer_len;
} Window;

// ====================
// GLOBAL VARIABLES
// ====================

static Entry entries[MAX_ENTRIES];
static int entry_count = 0;
static int cwd = 0;
static AuthSystem auth = {0};
static Window windows[MAX_WINDOWS];
static int window_count = 0;
static bool gui_mode = false;
static int next_window_id = 1;

#ifdef KERNEL_MODE
    static size_t terminal_row;
    static size_t terminal_column;
    static uint8_t terminal_color;
    static uint16_t* terminal_buffer;
    static bool shift_pressed = false;
#else
    static struct termios original_termios;
#endif

// ====================
// FORWARD DECLARATIONS
// ====================

void terminal_writestring(const char* data);
void clear_screen(void);
char* strchr(const char* s, int c);

// ====================
// I/O PORT FUNCTIONS (KERNEL ONLY)
// ====================

#ifdef KERNEL_MODE
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
#endif

// ====================
// STRING FUNCTIONS
// ====================

#ifdef KERNEL_MODE
// Custom implementations for kernel
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}

char* strcpy(char* dest, const char* src) {
    char* orig = dest;
    while ((*dest++ = *src++));
    return orig;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    while (n--)
        *p++ = (uint8_t)c;
    return s;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = s1;
    const uint8_t* p2 = s2;
    while (n--) {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == c) return (char*)s;
        s++;
    }
    return NULL;
}

int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    while (*str == ' ') str++;
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}
#endif

// ====================
// DISPLAY FUNCTIONS
// ====================

#ifdef KERNEL_MODE
    // VGA text mode functions
    static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
        return fg | bg << 4;
    }

    static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
        return (uint16_t)uc | (uint16_t)color << 8;
    }

    void terminal_initialize(void) {
        terminal_row = 0;
        terminal_column = 0;
        terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_buffer = (uint16_t*)VGA_MEMORY;
        for (size_t y = 0; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                const size_t index = y * VGA_WIDTH + x;
                terminal_buffer[index] = vga_entry(' ', terminal_color);
            }
        }
    }

    void terminal_setcolor(uint8_t color) {
        terminal_color = color;
    }

    void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
        const size_t index = y * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(c, color);
    }

    void terminal_scroll(void) {
        for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
            }
        }
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
        terminal_row = VGA_HEIGHT - 1;
    }

    void terminal_putchar(char c) {
        if (c == '\n') {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT)
                terminal_scroll();
            return;
        }
        
        if (c == '\b') {
            if (terminal_column > 0) {
                terminal_column--;
                terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
            }
            return;
        }
        
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT)
                terminal_scroll();
        }
    }

    void terminal_write(const char* data, size_t size) {
        for (size_t i = 0; i < size; i++)
            terminal_putchar(data[i]);
    }

    void terminal_writestring(const char* data) {
        terminal_write(data, strlen(data));
    }

    void clear_screen(void) {
        terminal_initialize();
    }

#else
    // ANSI terminal functions for simulator
    void terminal_initialize(void) {
        printf("\033[2J\033[H"); // Clear screen and move cursor to home
    }

    void terminal_writestring(const char* data) {
        printf("%s", data);
    }

    void clear_screen(void) {
        printf("\033[2J\033[H");
        fflush(stdout);
    }

    void enable_raw_mode(void) {
        tcgetattr(STDIN_FILENO, &original_termios);
        struct termios raw = original_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    void disable_raw_mode(void) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
    }
#endif

// ====================
// KEYBOARD INPUT
// ====================

#ifdef KERNEL_MODE
    // PS/2 keyboard driver
    static const char scancode_to_ascii[128] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    };

    static const char scancode_to_ascii_shift[128] = {
        0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
        0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
        '*', 0, ' '
    };

    char keyboard_getchar(void) {
        uint8_t scancode;
        
        while (!(inb(KEYBOARD_STATUS_PORT) & 0x01));
        scancode = inb(KEYBOARD_DATA_PORT);
        
        if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
            shift_pressed = true;
            return 0;
        }
        
        if (scancode == (KEY_LSHIFT | 0x80) || scancode == (KEY_RSHIFT | 0x80)) {
            shift_pressed = false;
            return 0;
        }
        
        if (scancode & 0x80) return 0;
        
        if (scancode < 128) {
            if (shift_pressed)
                return scancode_to_ascii_shift[scancode];
            else
                return scancode_to_ascii[scancode];
        }
        
        return 0;
    }

    void read_line(char* buffer, size_t max_len) {
        size_t pos = 0;
        
        while (1) {
            char c = keyboard_getchar();
            
            if (c == 0) continue;
            
            if (c == '\n') {
                buffer[pos] = '\0';
                terminal_putchar('\n');
                return;
            }
            
            if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    terminal_putchar('\b');
                }
            } else if (pos < max_len - 1) {
                buffer[pos++] = c;
                terminal_putchar(c);
            }
        }
    }

    void read_password(char* buffer, size_t max_len) {
        size_t pos = 0;
        
        while (1) {
            char c = keyboard_getchar();
            
            if (c == 0) continue;
            
            if (c == '\n') {
                buffer[pos] = '\0';
                terminal_putchar('\n');
                return;
            }
            
            if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    terminal_putchar('\b');
                }
            } else if (pos < max_len - 1) {
                buffer[pos++] = c;
                terminal_putchar('*');
            }
        }
    }

#else
    // Standard getchar for simulator
    void read_line(char* buffer, size_t max_len) {
        if (fgets(buffer, max_len, stdin)) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n')
                buffer[len-1] = '\0';
        }
    }

    void read_password(char* buffer, size_t max_len) {
        struct termios old, new;
        tcgetattr(STDIN_FILENO, &old);
        new = old;
        new.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &new);
        
        if (fgets(buffer, max_len, stdin)) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n')
                buffer[len-1] = '\0';
        }
        
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        printf("\n");
    }
#endif

// ====================
// CRYPTO FUNCTIONS
// ====================

void sha256_simple(const char* input, uint8_t* output) {
    uint32_t hash = 5381;
    const char* str = input;
    
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        output[i] = (uint8_t)((hash >> (i % 8)) & 0xFF);
        hash = hash * 33 + i;
    }
}

// ====================
// FILESYSTEM
// ====================

int alloc_entry(const char* name, EntryType type, int parent) {
    if (entry_count >= MAX_ENTRIES) return -1;
    int idx = entry_count++;
    memset(&entries[idx], 0, sizeof(Entry));
    strncpy(entries[idx].name, name, MAX_NAME - 1);
    entries[idx].type = type;
    entries[idx].parent = parent;
    entries[idx].first_child = -1;
    entries[idx].next_sibling = -1;
    return idx;
}

int find_child_by_name(int dir_idx, const char* name) {
    if (dir_idx < 0 || dir_idx >= entry_count) return -1;
    int i = entries[dir_idx].first_child;
    while (i != -1) {
        if (strcmp(entries[i].name, name) == 0) return i;
        i = entries[i].next_sibling;
    }
    return -1;
}

int add_child(int dir_idx, int child_idx) {
    if (dir_idx < 0 || dir_idx >= entry_count) return -1;
    entries[child_idx].next_sibling = entries[dir_idx].first_child;
    entries[dir_idx].first_child = child_idx;
    return 0;
}

void init_fs(void) {
    entry_count = 0;
    int root = alloc_entry("/", DIR_ENTRY, -1);
    cwd = root;
}

// ====================
// AUTHENTICATION
// ====================

void init_auth(void) {
    memset(&auth, 0, sizeof(AuthSystem));
    auth.user_count = 0;
    auth.current_user = -1;
    auth.logged_in = false;
    
    User* admin = &auth.users[auth.user_count];
    strncpy(admin->username, "admin", MAX_NAME - 1);
    sha256_simple("slopOS123", admin->password_hash);
    admin->is_admin = true;
    admin->active = true;
    auth.user_count++;
    
    User* user = &auth.users[auth.user_count];
    strncpy(user->username, "user", MAX_NAME - 1);
    sha256_simple("password", user->password_hash);
    user->is_admin = false;
    user->active = true;
    auth.user_count++;
}

int find_user(const char* username) {
    for (int i = 0; i < auth.user_count; i++) {
        if (strcmp(auth.users[i].username, username) == 0 && auth.users[i].active) {
            return i;
        }
    }
    return -1;
}

bool verify_password(int user_idx, const char* password) {
    if (user_idx < 0 || user_idx >= auth.user_count) return false;
    
    uint8_t input_hash[SHA256_DIGEST_LENGTH];
    sha256_simple(password, input_hash);
    
    return memcmp(auth.users[user_idx].password_hash, input_hash, SHA256_DIGEST_LENGTH) == 0;
}

bool login(void) {
    char username[MAX_NAME];
    char password[MAX_PASSWORD];
    int attempts = 0;
    const int max_attempts = 3;
    
    #ifdef KERNEL_MODE
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    #else
        printf(COLOR_CYAN);
    #endif
    
    terminal_writestring("=== slopOS Login ===\n");
    
    #ifdef KERNEL_MODE
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    #else
        printf(COLOR_RESET);
    #endif
    
    terminal_writestring("Default users:\n");
    terminal_writestring("  admin / slopOS123 (administrator)\n");
    terminal_writestring("  user  / password  (regular user)\n\n");
    
    while (attempts < max_attempts) {
        terminal_writestring("Username: ");
        read_line(username, sizeof(username));
        
        terminal_writestring("Password: ");
        read_password(password, sizeof(password));
        
        int user_idx = find_user(username);
        if (user_idx != -1 && verify_password(user_idx, password)) {
            auth.current_user = user_idx;
            auth.logged_in = true;
            
            #ifdef KERNEL_MODE
                terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            #else
                printf(COLOR_GREEN);
            #endif
            
            terminal_writestring("Login successful! Welcome, ");
            terminal_writestring(auth.users[user_idx].username);
            terminal_writestring(".\n");
            
            if (auth.users[user_idx].is_admin) {
                terminal_writestring("Administrator privileges granted.\n");
            }
            
            #ifdef KERNEL_MODE
                terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
            #else
                printf(COLOR_RESET);
            #endif
            
            terminal_writestring("\n");
            memset(password, 0, sizeof(password));
            return true;
        }
        
        attempts++;
        
        #ifdef KERNEL_MODE
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        #else
            printf(COLOR_RED);
        #endif
        
        terminal_writestring("Invalid username or password.\n");
        
        #ifdef KERNEL_MODE
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        #else
            printf(COLOR_RESET);
        #endif
        
        memset(password, 0, sizeof(password));
    }
    
    #ifdef KERNEL_MODE
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    #else
        printf(COLOR_RED);
    #endif
    
    terminal_writestring("Too many failed attempts. System halted.\n");
    return false;
}

const char* get_current_username(void) {
    if (auth.logged_in && auth.current_user >= 0) {
        return auth.users[auth.current_user].username;
    }
    return "unknown";
}

bool is_current_user_admin(void) {
    if (auth.logged_in && auth.current_user >= 0) {
        return auth.users[auth.current_user].is_admin;
    }
    return false;
}

// ====================
// COMMANDS
// ====================

void cmd_help(void) {
    terminal_writestring("Available commands:\n");
    terminal_writestring("  ls             - list directory contents\n");
    terminal_writestring("  mkdir <name>   - create directory\n");
    terminal_writestring("  touch <name>   - create file\n");
    terminal_writestring("  cd <name|..>   - change directory\n");
    terminal_writestring("  pwd            - print working directory\n");
    terminal_writestring("  whoami         - show current user\n");
    if (is_current_user_admin()) {
        terminal_writestring("  listusers      - list all users (admin)\n");
        #ifndef KERNEL_MODE
            terminal_writestring("  adduser        - add new user (admin)\n");
            terminal_writestring("  deluser        - delete user (admin)\n");
        #endif
    }
    #ifndef KERNEL_MODE
        terminal_writestring("  passwd         - change password\n");
        terminal_writestring("  startgui       - start GUI mode\n");
    #endif
    terminal_writestring("  clear          - clear screen\n");
    terminal_writestring("  help           - show this help\n");
}

void cmd_ls(void) {
    int i = entries[cwd].first_child;
    if (i == -1) {
        terminal_writestring("(empty directory)\n");
        return;
    }
    while (i != -1) {
        char prefix[4];
        prefix[0] = entries[i].type == DIR_ENTRY ? 'd' : '-';
        prefix[1] = ' ';
        prefix[2] = '\0';
        terminal_writestring(prefix);
        terminal_writestring(entries[i].name);
        terminal_writestring("\n");
        i = entries[i].next_sibling;
    }
}

void cmd_mkdir(const char* name) {
    if (find_child_by_name(cwd, name) != -1) {
        terminal_writestring("Error: entry already exists\n");
        return;
    }
    int idx = alloc_entry(name, DIR_ENTRY, cwd);
    if (idx < 0) {
        terminal_writestring("Error: filesystem full\n");
        return;
    }
    add_child(cwd, idx);
    terminal_writestring("Directory created\n");
}

void cmd_touch(const char* name) {
    if (find_child_by_name(cwd, name) != -1) {
        terminal_writestring("Error: entry already exists\n");
        return;
    }
    int idx = alloc_entry(name, FILE_ENTRY, cwd);
    if (idx < 0) {
        terminal_writestring("Error: filesystem full\n");
        return;
    }
    add_child(cwd, idx);
    terminal_writestring("File created\n");
}

void cmd_cd(const char* name) {
    if (strcmp(name, "..") == 0) {
        if (entries[cwd].parent != -1)
            cwd = entries[cwd].parent;
        return;
    }
    int idx = find_child_by_name(cwd, name);
    if (idx == -1) {
        terminal_writestring("No such directory\n");
        return;
    }
    if (entries[idx].type != DIR_ENTRY) {
        terminal_writestring("Not a directory\n");
        return;
    }
    cwd = idx;
}

void cmd_pwd_recursive(int idx) {
    if (entries[idx].parent != -1) {
        cmd_pwd_recursive(entries[idx].parent);
        if (idx != 0) {
            terminal_writestring("/");
            terminal_writestring(entries[idx].name);
        }
    } else {
        terminal_writestring("/");
    }
}

void cmd_pwd(void) {
    cmd_pwd_recursive(cwd);
    terminal_writestring("\n");
}

void cmd_whoami(void) {
    terminal_writestring(get_current_username());
    if (is_current_user_admin()) {
        terminal_writestring(" (administrator)");
    }
    terminal_writestring("\n");
}

void cmd_listusers(void) {
    if (!is_current_user_admin()) {
        terminal_writestring("Permission denied\n");
        return;
    }
    
    terminal_writestring("Active users:\n");
    for (int i = 0; i < auth.user_count; i++) {
        if (auth.users[i].active) {
            terminal_writestring("  ");
            terminal_writestring(auth.users[i].username);
            terminal_writestring(auth.users[i].is_admin ? " (admin)" : " (user)");
            if (i == auth.current_user)
                terminal_writestring(" [current]");
            terminal_writestring("\n");
        }
    }
}

#ifndef KERNEL_MODE
// Simulator-only commands

void cmd_passwd(void) {
    char oldpass[MAX_PASSWORD];
    char newpass[MAX_PASSWORD];
    char confirm[MAX_PASSWORD];
    
    printf("Old password: ");
    read_password(oldpass, sizeof(oldpass));
    
    if (!verify_password(auth.current_user, oldpass)) {
        printf(COLOR_RED "Incorrect password\n" COLOR_RESET);
        memset(oldpass, 0, sizeof(oldpass));
        return;
    }
    
    printf("New password: ");
    read_password(newpass, sizeof(newpass));
    
    printf("Confirm password: ");
    read_password(confirm, sizeof(confirm));
    
    if (strcmp(newpass, confirm) != 0) {
        printf(COLOR_RED "Passwords do not match\n" COLOR_RESET);
        memset(oldpass, 0, sizeof(oldpass));
        memset(newpass, 0, sizeof(newpass));
        memset(confirm, 0, sizeof(confirm));
        return;
    }
    
    sha256_simple(newpass, auth.users[auth.current_user].password_hash);
    printf(COLOR_GREEN "Password changed successfully\n" COLOR_RESET);
    
    memset(oldpass, 0, sizeof(oldpass));
    memset(newpass, 0, sizeof(newpass));
    memset(confirm, 0, sizeof(confirm));
}

void cmd_adduser(const char* username, const char* password) {
    if (!is_current_user_admin()) {
        printf(COLOR_RED "Permission denied\n" COLOR_RESET);
        return;
    }
    
    if (auth.user_count >= MAX_USERS) {
        printf(COLOR_RED "Maximum users reached\n" COLOR_RESET);
        return;
    }
    
    if (find_user(username) != -1) {
        printf(COLOR_RED "User already exists\n" COLOR_RESET);
        return;
    }
    
    User* new_user = &auth.users[auth.user_count];
    strncpy(new_user->username, username, MAX_NAME - 1);
    sha256_simple(password, new_user->password_hash);
    new_user->is_admin = false;
    new_user->active = true;
    auth.user_count++;
    
    printf(COLOR_GREEN "User created successfully\n" COLOR_RESET);
}

void cmd_deluser(const char* username) {
    if (!is_current_user_admin()) {
        printf(COLOR_RED "Permission denied\n" COLOR_RESET);
        return;
    }
    
    if (strcmp(username, get_current_username()) == 0) {
        printf(COLOR_RED "Cannot delete current user\n" COLOR_RESET);
        return;
    }
    
    int user_idx = find_user(username);
    if (user_idx == -1) {
        printf(COLOR_RED "User not found\n" COLOR_RESET);
        return;
    }
    
    auth.users[user_idx].active = false;
    printf(COLOR_GREEN "User deleted successfully\n" COLOR_RESET);
}

void cmd_startgui(void) {
    printf(COLOR_YELLOW "GUI mode not fully implemented in this version\n" COLOR_RESET);
    printf("GUI commands: newwin, closewin, focuswin, listwin, writewin, exitgui\n");
    gui_mode = true;
}

void cmd_exitgui(void) {
    gui_mode = false;
    printf(COLOR_GREEN "Exited GUI mode\n" COLOR_RESET);
}

void cmd_newwin(const char* title, int x, int y, int w, int h) {
    if (window_count >= MAX_WINDOWS) {
        printf(COLOR_RED "Maximum windows reached\n" COLOR_RESET);
        return;
    }
    
    Window* win = &windows[window_count++];
    win->id = next_window_id++;
    strncpy(win->title, title, MAX_NAME - 1);
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->visible = true;
    win->focused = true;
    win->buffer[0] = '\0';
    win->buffer_len = 0;
    
    for (int i = 0; i < window_count - 1; i++) {
        windows[i].focused = false;
    }
    
    printf(COLOR_GREEN "Window %d created\n" COLOR_RESET, win->id);
}

void cmd_closewin(int id) {
    for (int i = 0; i < window_count; i++) {
        if (windows[i].id == id) {
            for (int j = i; j < window_count - 1; j++) {
                windows[j] = windows[j + 1];
            }
            window_count--;
            printf(COLOR_GREEN "Window closed\n" COLOR_RESET);
            return;
        }
    }
    printf(COLOR_RED "Window not found\n" COLOR_RESET);
}

void cmd_focuswin(int id) {
    for (int i = 0; i < window_count; i++) {
        windows[i].focused = (windows[i].id == id);
        if (windows[i].id == id) {
            printf(COLOR_GREEN "Window %d focused\n" COLOR_RESET, id);
            return;
        }
    }
    printf(COLOR_RED "Window not found\n" COLOR_RESET);
}

void cmd_listwin(void) {
    if (window_count == 0) {
        printf("No windows\n");
        return;
    }
    printf("Windows:\n");
    for (int i = 0; i < window_count; i++) {
        printf("  [%d] %s (%dx%d at %d,%d)%s\n",
               windows[i].id, windows[i].title,
               windows[i].width, windows[i].height,
               windows[i].x, windows[i].y,
               windows[i].focused ? " *focused*" : "");
    }
}

void cmd_writewin(int id, const char* text) {
    for (int i = 0; i < window_count; i++) {
        if (windows[i].id == id) {
            strncpy(windows[i].buffer, text, sizeof(windows[i].buffer) - 1);
            windows[i].buffer_len = strlen(windows[i].buffer);
            printf(COLOR_GREEN "Text written to window %d\n" COLOR_RESET, id);
            return;
        }
    }
    printf(COLOR_RED "Window not found\n" COLOR_RESET);
}
#endif

// ====================
// COMMAND PARSER
// ====================

char* strtok_state = NULL;

char* strtok(char* str, const char* delim) {
    if (str != NULL)
        strtok_state = str;
    
    if (strtok_state == NULL)
        return NULL;
    
    while (*strtok_state && strchr(delim, *strtok_state))
        strtok_state++;
    
    if (*strtok_state == '\0') {
        strtok_state = NULL;
        return NULL;
    }
    
    char* token_start = strtok_state;
    
    while (*strtok_state && !strchr(delim, *strtok_state))
        strtok_state++;
    
    if (*strtok_state) {
        *strtok_state = '\0';
        strtok_state++;
    } else {
        strtok_state = NULL;
    }
    
    return token_start;
}

void process_command(char* line) {
    char* cmd = strtok(line, " ");
    if (!cmd) return;
    
    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd, "mkdir") == 0) {
        char* arg = strtok(NULL, " ");
        if (arg) cmd_mkdir(arg);
        else terminal_writestring("mkdir: missing name\n");
    } else if (strcmp(cmd, "touch") == 0) {
        char* arg = strtok(NULL, " ");
        if (arg) cmd_touch(arg);
        else terminal_writestring("touch: missing name\n");
    } else if (strcmp(cmd, "cd") == 0) {
        char* arg = strtok(NULL, " ");
        if (arg) cmd_cd(arg);
        else terminal_writestring("cd: missing name\n");
    } else if (strcmp(cmd, "pwd") == 0) {
        cmd_pwd();
    } else if (strcmp(cmd, "whoami") == 0) {
        cmd_whoami();
    } else if (strcmp(cmd, "listusers") == 0) {
        cmd_listusers();
    } else if (strcmp(cmd, "clear") == 0) {
        clear_screen();
    }
#ifndef KERNEL_MODE
    else if (strcmp(cmd, "passwd") == 0) {
        cmd_passwd();
    } else if (strcmp(cmd, "adduser") == 0) {
        char* user = strtok(NULL, " ");
        char* pass = strtok(NULL, " ");
        if (user && pass) cmd_adduser(user, pass);
        else terminal_writestring("adduser: missing username or password\n");
    } else if (strcmp(cmd, "deluser") == 0) {
        char* user = strtok(NULL, " ");
        if (user) cmd_deluser(user);
        else terminal_writestring("deluser: missing username\n");
    } else if (strcmp(cmd, "startgui") == 0) {
        cmd_startgui();
    } else if (strcmp(cmd, "exitgui") == 0) {
        cmd_exitgui();
    } else if (strcmp(cmd, "newwin") == 0) {
        char* title = strtok(NULL, " ");
        char* xs = strtok(NULL, " ");
        char* ys = strtok(NULL, " ");
        char* ws = strtok(NULL, " ");
        char* hs = strtok(NULL, " ");
        if (title && xs && ys && ws && hs)
            cmd_newwin(title, atoi(xs), atoi(ys), atoi(ws), atoi(hs));
        else terminal_writestring("newwin: usage: newwin <title> <x> <y> <w> <h>\n");
    } else if (strcmp(cmd, "closewin") == 0) {
        char* id = strtok(NULL, " ");
        if (id) cmd_closewin(atoi(id));
        else terminal_writestring("closewin: missing window id\n");
    } else if (strcmp(cmd, "focuswin") == 0) {
        char* id = strtok(NULL, " ");
        if (id) cmd_focuswin(atoi(id));
        else terminal_writestring("focuswin: missing window id\n");
    } else if (strcmp(cmd, "listwin") == 0) {
        cmd_listwin();
    } else if (strcmp(cmd, "writewin") == 0) {
        char* id = strtok(NULL, " ");
        char* text = strtok(NULL, "");
        if (id && text) cmd_writewin(atoi(id), text);
        else terminal_writestring("writewin: usage: writewin <id> <text>\n");
    }
#endif
    else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(cmd);
        terminal_writestring("\n");
    }
}

// ====================
// MAIN FUNCTION
// ====================

#ifdef KERNEL_MODE
void kernel_main(void) {
    terminal_initialize();
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("slopOS v0.1.0 - Full Featured Bootable OS\n");
    terminal_writestring("==========================================\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("Initializing system...\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("  [OK] VGA display initialized\n");
    terminal_writestring("  [OK] Keyboard driver loaded\n");
    
    init_fs();
    terminal_writestring("  [OK] Filesystem initialized\n");
    
    init_auth();
    terminal_writestring("  [OK] Authentication system ready\n");
    terminal_writestring("\n");
    
    if (!login()) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("\nAuthentication failed. System halted.\n");
        while (1) __asm__ volatile ("hlt");
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("slopOS ready. Type 'help' for available commands.\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("\n");
    
    char line[256];
    while (1) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_writestring(get_current_username());
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_writestring("@slopOS:");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK));
        terminal_writestring(entries[cwd].name);
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_writestring("$ ");
        
        read_line(line, sizeof(line));
        
        if (line[0] != '\0') {
            process_command(line);
        }
    }
}

#else
int main(void) {
    terminal_initialize();
    
    printf(COLOR_CYAN "slopOS v0.1.0 - Simulator Mode\n");
    printf("===============================\n\n" COLOR_RESET);
    
    printf(COLOR_GREEN "Initializing system...\n" COLOR_RESET);
    printf("  [OK] Terminal initialized\n");
    printf("  [OK] Keyboard input ready\n");
    
    init_fs();
    printf("  [OK] Filesystem initialized\n");
    
    init_auth();
    printf("  [OK] Authentication system ready\n");
    printf("\n");
    
    if (!login()) {
        printf(COLOR_RED "\nAuthentication failed. Exiting.\n" COLOR_RESET);
        return 1;
    }
    
    printf(COLOR_GREEN "slopOS ready. Type 'help' for available commands.\n" COLOR_RESET);
    printf("\n");
    
    char line[256];
    while (1) {
        printf(COLOR_CYAN "%s" COLOR_RESET "@slopOS:" COLOR_BLUE "%s" COLOR_RESET "$ ",
               get_current_username(), entries[cwd].name);
        
        read_line(line, sizeof(line));
        
        if (line[0] != '\0') {
            process_command(line);
        }
    }
    
    return 0;
}
#endif
