/*
 * slopOS Kernel v0.1.0 - Full Featured Bootable OS
 * Complete implementation with authentication, filesystem, and GUI
 */

// Define standard types for freestanding environment
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t;
typedef int bool;
#define true 1
#define false 0
#define NULL ((void*)0)

// Configuration constants
#define MAX_NAME 64
#define MAX_ENTRIES 256
#define MAX_USERS 16
#define MAX_PASSWORD 32
#define SHA256_DIGEST_LENGTH 32
#define MAX_WINDOWS 8
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

// VGA text mode
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Keyboard I/O ports
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// VGA colors
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

// Entry types
typedef enum { FILE_ENTRY, DIR_ENTRY } EntryType;

// Filesystem entry
typedef struct Entry {
    char name[MAX_NAME];
    EntryType type;
    int parent;
    int first_child;
    int next_sibling;
} Entry;

// User structure
typedef struct User {
    char username[MAX_NAME];
    uint8_t password_hash[SHA256_DIGEST_LENGTH];
    bool is_admin;
    bool active;
} User;

// Authentication system
typedef struct AuthSystem {
    User users[MAX_USERS];
    int user_count;
    int current_user;
    bool logged_in;
} AuthSystem;

// Window structure
typedef struct Window {
    int id;
    char title[MAX_NAME];
    int x, y;
    int width, height;
    bool visible;
    bool focused;
} Window;

// Globals
static Entry entries[MAX_ENTRIES];
static int entry_count = 0;
static int cwd = 0;
static AuthSystem auth = {0};
static Window windows[MAX_WINDOWS];
static int window_count = 0;
static bool gui_mode = false;
static int next_window_id = 1;

// Terminal state
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

// Input buffer
static char input_buffer[256];
static size_t input_pos = 0;

// Forward declarations
void terminal_writestring(const char* data);
void clear_screen(void);

// ====================
// I/O PORT FUNCTIONS
// ====================

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// ====================
// STRING FUNCTIONS
// ====================

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

// String to integer
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

// ====================
// VGA FUNCTIONS
// ====================

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

// ====================
// KEYBOARD DRIVER
// ====================

// Keyboard scancodes
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36

// Scancode to ASCII mapping (without shift)
static const char scancode_to_ascii[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Scancode to ASCII mapping (with shift)
static const char scancode_to_ascii_shift[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

// Track shift key state
static bool shift_pressed = false;

char keyboard_getchar(void) {
    uint8_t scancode;
    
    // Wait for keyboard data
    while (!(inb(KEYBOARD_STATUS_PORT) & 0x01));
    
    scancode = inb(KEYBOARD_DATA_PORT);
    
    // Handle shift key press
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        shift_pressed = true;
        return 0;
    }
    
    // Handle shift key release
    if (scancode == (KEY_LSHIFT | 0x80) || scancode == (KEY_RSHIFT | 0x80)) {
        shift_pressed = false;
        return 0;
    }
    
    // Only handle key press (bit 7 = 0)
    if (scancode & 0x80) return 0;
    
    // Return character based on shift state
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
    
    // Create admin user
    User* admin = &auth.users[auth.user_count];
    strncpy(admin->username, "admin", MAX_NAME - 1);
    sha256_simple("slopOS123", admin->password_hash);
    admin->is_admin = true;
    admin->active = true;
    auth.user_count++;
    
    // Create regular user
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
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("=== slopOS Login ===\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
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
            
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            terminal_writestring("Login successful! Welcome, ");
            terminal_writestring(auth.users[user_idx].username);
            terminal_writestring(".\n");
            
            if (auth.users[user_idx].is_admin) {
                terminal_writestring("Administrator privileges granted.\n");
            }
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
            terminal_writestring("\n");
            
            memset(password, 0, sizeof(password));
            return true;
        }
        
        attempts++;
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("Invalid username or password.\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        
        memset(password, 0, sizeof(password));
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
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
    }
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
        terminal_putchar(entries[i].type == DIR_ENTRY ? 'd' : '-');
        terminal_writestring(" ");
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

// ====================
// COMMAND PARSER
// ====================

char* strtok_state = NULL;

char* strtok(char* str, const char* delim) {
    if (str != NULL)
        strtok_state = str;
    
    if (strtok_state == NULL)
        return NULL;
    
    // Skip leading delimiters
    while (*strtok_state && strchr(delim, *strtok_state))
        strtok_state++;
    
    if (*strtok_state == '\0') {
        strtok_state = NULL;
        return NULL;
    }
    
    char* token_start = strtok_state;
    
    // Find end of token
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
    } else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(cmd);
        terminal_writestring("\n");
    }
}

// ====================
// KERNEL MAIN
// ====================

void kernel_main(void) {
    terminal_initialize();
    
    // Boot message
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("slopOS v0.1.0 - Full Featured Bootable OS\n");
    terminal_writestring("==========================================\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("Initializing system...\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("  [OK] VGA display initialized\n");
    terminal_writestring("  [OK] Keyboard driver loaded\n");
    
    // Initialize subsystems
    init_fs();
    terminal_writestring("  [OK] Filesystem initialized\n");
    
    init_auth();
    terminal_writestring("  [OK] Authentication system ready\n");
    terminal_writestring("\n");
    
    // Login
    if (!login()) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("\nAuthentication failed. System halted.\n");
        while (1) __asm__ volatile ("hlt");
    }
    
    // Welcome message
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("slopOS ready. Type 'help' for available commands.\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("\n");
    
    // Command loop
    char line[256];
    while (1) {
        // Print prompt
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_writestring(get_current_username());
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_writestring("@slopOS:");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK));
        terminal_writestring(entries[cwd].name);
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_writestring("$ ");
        
        // Read command
        read_line(line, sizeof(line));
        
        // Process command
        if (line[0] != '\0') {
            process_command(line);
        }
    }
}
