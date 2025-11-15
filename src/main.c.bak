/*
 * slopOS v0.1.0 - Secure Operating System Simulator with Authentication
 * This is not a real OS bootloader/kernel; it's a user-space simulator
 * that demonstrates a secure filesystem, user management, and GUI.
 *
 * Features:
 *  - User authentication with SHA256 password hashing
 *  - Multi-user support with admin/user roles
 *  - Simple in-memory proprietary filesystem (flat + directories)
 *  - Commands: mkdir, touch, ls, help, exit, cd, pwd, whoami, passwd
 *  - Admin commands: adduser, listusers, deluser
 *  - Text-based Window Manager with overlapping windows
 *  - GUI Commands: startgui, newwin, closewin, focuswin, listwin, writewin, exitgui
 *
 * Default users:
 *  - admin / slopOS123 (administrator)
 *  - user  / password  (regular user)
 *
 * Build: gcc src/main.c -o slopos_sim
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <termios.h>

#define MAX_NAME 64
#define MAX_ENTRIES 1024
#define MAX_WINDOWS 16
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 24
#define MAX_USERS 16
#define MAX_PASSWORD 32
#define SHA256_DIGEST_LENGTH 32

typedef enum { FILE_ENTRY, DIR_ENTRY } EntryType;

// User management structures
typedef struct User {
    char username[MAX_NAME];
    unsigned char password_hash[SHA256_DIGEST_LENGTH];
    bool is_admin;
    bool active;
} User;

typedef struct AuthSystem {
    User users[MAX_USERS];
    int user_count;
    int current_user;
    bool logged_in;
} AuthSystem;

typedef struct Entry {
    char name[MAX_NAME];
    EntryType type;
    int parent; // index of parent directory, -1 for root
    int first_child; // index of first child in linked list (-1 if none)
    int next_sibling; // index of next sibling in parent's child list
    char *data; // file data (malloc'd), NULL for directories
    size_t size;
} Entry;

// Window Manager structures
typedef struct Window {
    int id;
    char title[MAX_NAME];
    int x, y;           // position
    int width, height;  // dimensions
    bool visible;
    bool focused;
    char **buffer;      // text buffer for window content
    int buffer_lines;
} Window;

typedef struct WindowManager {
    Window windows[MAX_WINDOWS];
    int window_count;
    int focused_window;
    bool gui_mode;
    char screen[SCREEN_HEIGHT][SCREEN_WIDTH + 1];
} WindowManager;

static Entry entries[MAX_ENTRIES];
static int entry_count = 0;
static int cwd = 0; // index of current working directory

// Authentication system
static AuthSystem auth = {0};

// Window Manager globals
static WindowManager wm = {0};
static int next_window_id = 1;

int alloc_entry(const char *name, EntryType type, int parent) {
    if (entry_count >= MAX_ENTRIES) return -1;
    int idx = entry_count++;
    memset(&entries[idx], 0, sizeof(Entry));
    strncpy(entries[idx].name, name, MAX_NAME-1);
    entries[idx].type = type;
    entries[idx].parent = parent;
    entries[idx].first_child = -1;
    entries[idx].next_sibling = -1;
    entries[idx].data = NULL;
    entries[idx].size = 0;
    return idx;
}

int find_child_by_name(int dir_idx, const char *name) {
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

void init_fs() {
    entry_count = 0;
    int root = alloc_entry("/", DIR_ENTRY, -1);
    cwd = root;
}

// Simple SHA256 implementation (simplified for educational purposes)
// In production, use a proper crypto library like OpenSSL
void sha256_simple(const char* input, unsigned char* output) {
    // This is a simplified hash function for demonstration
    // In a real OS, you'd use a proper SHA256 implementation
    unsigned int hash = 5381;
    const char* str = input;
    
    // Simple hash calculation
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    
    // Fill the output buffer with derived values
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        output[i] = (unsigned char)((hash >> (i % 8)) & 0xFF);
        hash = hash * 33 + i; // Add some variation
    }
}

// Secure password input (hide characters)
void get_password(char* password, size_t max_len) {
    struct termios old_termios, new_termios;
    
    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    
    // Disable echo
    new_termios.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    
    // Read password
    if (fgets(password, max_len, stdin)) {
        // Remove newline
        char* newline = strchr(password, '\n');
        if (newline) *newline = '\0';
    }
    
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    printf("\n");
}

// Initialize authentication system with default admin user
void init_auth() {
    memset(&auth, 0, sizeof(AuthSystem));
    auth.user_count = 0;
    auth.current_user = -1;
    auth.logged_in = false;
    
    // Create default admin user (username: admin, password: slopOS123)
    User* admin = &auth.users[auth.user_count];
    strncpy(admin->username, "admin", MAX_NAME - 1);
    sha256_simple("slopOS123", admin->password_hash);
    admin->is_admin = true;
    admin->active = true;
    auth.user_count++;
    
    // Create a regular user (username: user, password: password)
    User* user = &auth.users[auth.user_count];
    strncpy(user->username, "user", MAX_NAME - 1);
    sha256_simple("password", user->password_hash);
    user->is_admin = false;
    user->active = true;
    auth.user_count++;
}

// Find user by username
int find_user(const char* username) {
    for (int i = 0; i < auth.user_count; i++) {
        if (strcmp(auth.users[i].username, username) == 0 && auth.users[i].active) {
            return i;
        }
    }
    return -1;
}

// Verify password
bool verify_password(int user_idx, const char* password) {
    if (user_idx < 0 || user_idx >= auth.user_count) return false;
    
    unsigned char input_hash[SHA256_DIGEST_LENGTH];
    sha256_simple(password, input_hash);
    
    return memcmp(auth.users[user_idx].password_hash, input_hash, SHA256_DIGEST_LENGTH) == 0;
}

// Login function
bool login() {
    char username[MAX_NAME];
    char password[MAX_PASSWORD];
    int attempts = 0;
    const int max_attempts = 3;
    
    printf("=== slopOS Login ===\n");
    printf("Default users:\n");
    printf("  admin / slopOS123 (administrator)\n");
    printf("  user  / password  (regular user)\n\n");
    
    while (attempts < max_attempts) {
        printf("Username: ");
        if (!fgets(username, sizeof(username), stdin)) {
            return false;
        }
        
        // Remove newline
        char* newline = strchr(username, '\n');
        if (newline) *newline = '\0';
        
        printf("Password: ");
        get_password(password, sizeof(password));
        
        int user_idx = find_user(username);
        if (user_idx != -1 && verify_password(user_idx, password)) {
            auth.current_user = user_idx;
            auth.logged_in = true;
            printf("Login successful! Welcome, %s.\n", auth.users[user_idx].username);
            if (auth.users[user_idx].is_admin) {
                printf("Administrator privileges granted.\n");
            }
            printf("\n");
            
            // Clear password from memory
            memset(password, 0, sizeof(password));
            return true;
        }
        
        attempts++;
        printf("Invalid username or password. Attempts remaining: %d\n\n", max_attempts - attempts);
        
        // Clear password from memory
        memset(password, 0, sizeof(password));
    }
    
    printf("Too many failed attempts. Access denied.\n");
    return false;
}

// Logout function
void logout() {
    if (auth.logged_in) {
        printf("Goodbye, %s!\n", auth.users[auth.current_user].username);
        auth.current_user = -1;
        auth.logged_in = false;
        
        // Exit GUI mode if active
        if (wm.gui_mode) {
            cmd_exitgui();
        }
    }
}

// Get current username
const char* get_current_username() {
    if (auth.logged_in && auth.current_user >= 0) {
        return auth.users[auth.current_user].username;
    }
    return "unknown";
}

// Check if current user is admin
bool is_current_user_admin() {
    if (auth.logged_in && auth.current_user >= 0) {
        return auth.users[auth.current_user].is_admin;
    }
    return false;
}

// Window Manager functions
void clear_screen() {
    printf("\033[2J\033[H"); // ANSI escape codes to clear screen and move cursor to home
}

void init_wm() {
    memset(&wm, 0, sizeof(WindowManager));
    wm.window_count = 0;
    wm.focused_window = -1;
    // Don't reset gui_mode here - it should be set by caller
    
    // Initialize screen buffer
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        memset(wm.screen[i], ' ', SCREEN_WIDTH);
        wm.screen[i][SCREEN_WIDTH] = '\0';
    }
}

Window* create_window(const char* title, int x, int y, int width, int height) {
    if (wm.window_count >= MAX_WINDOWS) return NULL;
    
    Window* win = &wm.windows[wm.window_count];
    win->id = next_window_id++;
    strncpy(win->title, title, MAX_NAME - 1);
    win->title[MAX_NAME - 1] = '\0';
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->visible = true;
    win->focused = false;
    win->buffer_lines = height - 2; // exclude title bar and border
    
    // Allocate buffer for window content
    win->buffer = malloc(win->buffer_lines * sizeof(char*));
    for (int i = 0; i < win->buffer_lines; i++) {
        win->buffer[i] = malloc(width - 1); // exclude border
        memset(win->buffer[i], ' ', width - 2);
        win->buffer[i][width - 2] = '\0';
    }
    
    wm.window_count++;
    wm.focused_window = wm.window_count - 1;
    wm.windows[wm.focused_window].focused = true;
    
    return win;
}

void draw_window(Window* win) {
    if (!win->visible) return;
    
    // Draw window border and title
    for (int row = 0; row < win->height; row++) {
        for (int col = 0; col < win->width; col++) {
            int screen_x = win->x + col;
            int screen_y = win->y + row;
            
            if (screen_x >= SCREEN_WIDTH || screen_y >= SCREEN_HEIGHT) continue;
            
            char ch = ' ';
            if (row == 0 || row == win->height - 1) {
                ch = (col == 0 || col == win->width - 1) ? '+' : '-';
            } else if (col == 0 || col == win->width - 1) {
                ch = '|';
            } else if (row > 0 && row < win->height - 1) {
                // Content area
                int content_row = row - 1;
                int content_col = col - 1;
                if (content_row < win->buffer_lines && content_col < strlen(win->buffer[content_row])) {
                    ch = win->buffer[content_row][content_col];
                }
            }
            
            wm.screen[screen_y][screen_x] = ch;
        }
    }
    
    // Draw title
    int title_start = win->x + 2;
    int title_len = strlen(win->title);
    for (int i = 0; i < title_len && title_start + i < win->x + win->width - 2; i++) {
        if (title_start + i < SCREEN_WIDTH) {
            wm.screen[win->y][title_start + i] = win->title[i];
        }
    }
    
    // Mark focused window with asterisks
    if (win->focused && win->x > 0) {
        wm.screen[win->y][win->x - 1] = '*';
        if (win->x + win->width < SCREEN_WIDTH) {
            wm.screen[win->y][win->x + win->width] = '*';
        }
    }
}

void render_screen() {
    // Clear screen buffer
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        memset(wm.screen[i], ' ', SCREEN_WIDTH);
        wm.screen[i][SCREEN_WIDTH] = '\0';
    }
    
    // Draw all windows
    for (int i = 0; i < wm.window_count; i++) {
        draw_window(&wm.windows[i]);
    }
    
    // Print screen buffer
    clear_screen();
    for (int i = 0; i < SCREEN_HEIGHT - 1; i++) {
        printf("%s\n", wm.screen[i]);
    }
    printf("GUI Mode - Commands: newwin, closewin, focuswin, listwin, exitgui, help\n");
}

void window_write(Window* win, int line, const char* text) {
    if (!win || line >= win->buffer_lines || line < 0) return;
    
    strncpy(win->buffer[line], text, win->width - 3);
    win->buffer[line][win->width - 3] = '\0';
}

void close_window(int window_id) {
    int idx = -1;
    for (int i = 0; i < wm.window_count; i++) {
        if (wm.windows[i].id == window_id) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) return;
    
    // Free window buffer
    for (int i = 0; i < wm.windows[idx].buffer_lines; i++) {
        free(wm.windows[idx].buffer[i]);
    }
    free(wm.windows[idx].buffer);
    
    // Shift remaining windows
    for (int i = idx; i < wm.window_count - 1; i++) {
        wm.windows[i] = wm.windows[i + 1];
    }
    wm.window_count--;
    
    // Update focused window
    if (wm.focused_window >= wm.window_count) {
        wm.focused_window = wm.window_count - 1;
    }
    if (wm.focused_window >= 0) {
        for (int i = 0; i < wm.window_count; i++) {
            wm.windows[i].focused = (i == wm.focused_window);
        }
    }
}

void focus_window(int window_id) {
    for (int i = 0; i < wm.window_count; i++) {
        wm.windows[i].focused = (wm.windows[i].id == window_id);
        if (wm.windows[i].id == window_id) {
            wm.focused_window = i;
        }
    }
}

void cmd_help() {
    if (wm.gui_mode) {
        printf("Available GUI commands:\n");
        printf("  newwin <title> <x> <y> <w> <h> - create new window\n");
        printf("  closewin <id>                  - close window by ID\n");
        printf("  focuswin <id>                  - focus window by ID\n");
        printf("  listwin                        - list all windows\n");
        printf("  writewin <id> <line> <text>    - write text to window\n");
        printf("  exitgui                        - exit GUI mode\n");
        printf("  help                           - show this help\n");
    } else {
        printf("Available commands:\n");
        printf("  mkdir <name>   - create directory in cwd\n");
        printf("  touch <name>   - create empty file in cwd\n");
        printf("  ls             - list entries in cwd\n");
        printf("  cd <name|..>   - change directory\n");
        printf("  pwd            - print current path\n");
        printf("  whoami         - show current user\n");
        if (is_current_user_admin()) {
            printf("  adduser <name> - add new user (admin only)\n");
            printf("  listusers      - list all users (admin only)\n");
            printf("  deluser <name> - delete user (admin only)\n");
        }
        printf("  passwd         - change password\n");
        printf("  startgui       - start GUI window manager\n");
        printf("  logout         - logout current user\n");
        printf("  help           - show this help\n");
        printf("  exit           - quit\n");
    }
}

void cmd_ls() {
    int i = entries[cwd].first_child;
    while (i != -1) {
        printf("%c %s\n", entries[i].type==DIR_ENTRY ? 'd' : '-', entries[i].name);
        i = entries[i].next_sibling;
    }
}

void cmd_mkdir(const char *name) {
    if (find_child_by_name(cwd, name) != -1) {
        printf("Error: entry '%s' already exists\n", name);
        return;
    }
    int idx = alloc_entry(name, DIR_ENTRY, cwd);
    if (idx < 0) { printf("Error: FS full\n"); return; }
    add_child(cwd, idx);
    printf("Directory '%s' created\n", name);
}

void cmd_touch(const char *name) {
    if (find_child_by_name(cwd, name) != -1) {
        printf("Error: entry '%s' already exists\n", name);
        return;
    }
    int idx = alloc_entry(name, FILE_ENTRY, cwd);
    if (idx < 0) { printf("Error: FS full\n"); return; }
    entries[idx].data = malloc(1);
    entries[idx].data[0] = '\0';
    entries[idx].size = 0;
    add_child(cwd, idx);
    printf("File '%s' created\n", name);
}

void cmd_cd(const char *name) {
    if (strcmp(name, "..") == 0) {
        if (entries[cwd].parent != -1) cwd = entries[cwd].parent;
        return;
    }
    int idx = find_child_by_name(cwd, name);
    if (idx == -1) { printf("No such directory: %s\n", name); return; }
    if (entries[idx].type != DIR_ENTRY) { printf("Not a directory: %s\n", name); return; }
    cwd = idx;
}

void cmd_pwd_recursive(int idx) {
    if (entries[idx].parent != -1) {
        cmd_pwd_recursive(entries[idx].parent);
        if (idx != 0) printf("/%s", entries[idx].name);
    } else {
        printf("/");
    }
}

void cmd_pwd() {
    cmd_pwd_recursive(cwd);
    printf("\n");
}

// User management commands
void cmd_whoami() {
    printf("%s", get_current_username());
    if (is_current_user_admin()) {
        printf(" (administrator)");
    }
    printf("\n");
}

void cmd_adduser(const char* username) {
    if (!is_current_user_admin()) {
        printf("Permission denied. Administrator privileges required.\n");
        return;
    }
    
    if (!username || strlen(username) == 0) {
        printf("Usage: adduser <username>\n");
        return;
    }
    
    if (find_user(username) != -1) {
        printf("User '%s' already exists.\n", username);
        return;
    }
    
    if (auth.user_count >= MAX_USERS) {
        printf("Maximum number of users reached.\n");
        return;
    }
    
    char password[MAX_PASSWORD];
    char confirm_password[MAX_PASSWORD];
    
    printf("Enter password for new user '%s': ", username);
    get_password(password, sizeof(password));
    
    printf("Confirm password: ");
    get_password(confirm_password, sizeof(confirm_password));
    
    if (strcmp(password, confirm_password) != 0) {
        printf("Passwords do not match.\n");
        memset(password, 0, sizeof(password));
        memset(confirm_password, 0, sizeof(confirm_password));
        return;
    }
    
    User* new_user = &auth.users[auth.user_count];
    strncpy(new_user->username, username, MAX_NAME - 1);
    new_user->username[MAX_NAME - 1] = '\0';
    sha256_simple(password, new_user->password_hash);
    new_user->is_admin = false;
    new_user->active = true;
    auth.user_count++;
    
    printf("User '%s' created successfully.\n", username);
    
    // Clear passwords from memory
    memset(password, 0, sizeof(password));
    memset(confirm_password, 0, sizeof(confirm_password));
}

void cmd_listusers() {
    if (!is_current_user_admin()) {
        printf("Permission denied. Administrator privileges required.\n");
        return;
    }
    
    printf("Active users:\n");
    for (int i = 0; i < auth.user_count; i++) {
        if (auth.users[i].active) {
            printf("  %s %s%s\n", 
                   auth.users[i].username,
                   auth.users[i].is_admin ? "(admin)" : "(user)",
                   (i == auth.current_user) ? " [current]" : "");
        }
    }
}

void cmd_deluser(const char* username) {
    if (!is_current_user_admin()) {
        printf("Permission denied. Administrator privileges required.\n");
        return;
    }
    
    if (!username || strlen(username) == 0) {
        printf("Usage: deluser <username>\n");
        return;
    }
    
    if (strcmp(username, get_current_username()) == 0) {
        printf("Cannot delete your own account.\n");
        return;
    }
    
    int user_idx = find_user(username);
    if (user_idx == -1) {
        printf("User '%s' not found.\n", username);
        return;
    }
    
    auth.users[user_idx].active = false;
    printf("User '%s' deleted.\n", username);
}

void cmd_passwd() {
    char old_password[MAX_PASSWORD];
    char new_password[MAX_PASSWORD];
    char confirm_password[MAX_PASSWORD];
    
    printf("Enter current password: ");
    get_password(old_password, sizeof(old_password));
    
    if (!verify_password(auth.current_user, old_password)) {
        printf("Incorrect current password.\n");
        memset(old_password, 0, sizeof(old_password));
        return;
    }
    
    printf("Enter new password: ");
    get_password(new_password, sizeof(new_password));
    
    printf("Confirm new password: ");
    get_password(confirm_password, sizeof(confirm_password));
    
    if (strcmp(new_password, confirm_password) != 0) {
        printf("Passwords do not match.\n");
        memset(old_password, 0, sizeof(old_password));
        memset(new_password, 0, sizeof(new_password));
        memset(confirm_password, 0, sizeof(confirm_password));
        return;
    }
    
    sha256_simple(new_password, auth.users[auth.current_user].password_hash);
    printf("Password changed successfully.\n");
    
    // Clear passwords from memory
    memset(old_password, 0, sizeof(old_password));
    memset(new_password, 0, sizeof(new_password));
    memset(confirm_password, 0, sizeof(confirm_password));
}

// GUI Commands
void cmd_startgui() {
    init_wm();  // Initialize first
    wm.gui_mode = true;  // Then set GUI mode
    
    // Create a welcome window
    Window* welcome = create_window("Welcome to slopOS GUI", 10, 5, 40, 8);
    if (welcome) {
        window_write(welcome, 0, "Welcome to slopOS Window Manager!");
        window_write(welcome, 1, "");
        window_write(welcome, 2, "Try these commands:");
        window_write(welcome, 3, "  newwin Demo 2 2 30 6");
        window_write(welcome, 4, "  listwin");
        window_write(welcome, 5, "  help");
    }
    
    render_screen();
}

void cmd_newwin(char* args) {
    char title[MAX_NAME] = "Untitled";
    int x = 0, y = 0, width = 20, height = 10;
    
    if (args) {
        char* title_arg = strtok(args, " ");
        char* x_arg = strtok(NULL, " ");
        char* y_arg = strtok(NULL, " ");
        char* w_arg = strtok(NULL, " ");
        char* h_arg = strtok(NULL, " ");
        
        if (title_arg) strncpy(title, title_arg, MAX_NAME - 1);
        if (x_arg) x = atoi(x_arg);
        if (y_arg) y = atoi(y_arg);
        if (w_arg) width = atoi(w_arg);
        if (h_arg) height = atoi(h_arg);
    }
    
    // Bounds checking
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + width > SCREEN_WIDTH) width = SCREEN_WIDTH - x;
    if (y + height > SCREEN_HEIGHT - 1) height = SCREEN_HEIGHT - 1 - y;
    if (width < 10) width = 10;
    if (height < 4) height = 4;
    
    Window* win = create_window(title, x, y, width, height);
    if (win) {
        window_write(win, 0, "New window created!");
        window_write(win, 1, "Window ID: ");
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%d", win->id);
        strncat(win->buffer[1], id_str, width - 15);
        
        render_screen();
        printf("Created window ID %d\n", win->id);
    } else {
        printf("Failed to create window (max %d windows)\n", MAX_WINDOWS);
    }
}

void cmd_closewin(int window_id) {
    close_window(window_id);
    render_screen();
    printf("Window %d closed\n", window_id);
}

void cmd_focuswin(int window_id) {
    focus_window(window_id);
    render_screen();
    printf("Focused window %d\n", window_id);
}

void cmd_listwin() {
    printf("Active windows:\n");
    for (int i = 0; i < wm.window_count; i++) {
        Window* win = &wm.windows[i];
        printf("  ID: %d, Title: '%s', Pos: (%d,%d), Size: %dx%d %s\n", 
               win->id, win->title, win->x, win->y, win->width, win->height,
               win->focused ? "[FOCUSED]" : "");
    }
}

void cmd_writewin(char* args) {
    if (!args) {
        printf("Usage: writewin <id> <line> <text>\n");
        return;
    }
    
    char* id_str = strtok(args, " ");
    char* line_str = strtok(NULL, " ");
    char* text = strtok(NULL, ""); // Rest of the string
    
    if (!id_str || !line_str || !text) {
        printf("Usage: writewin <id> <line> <text>\n");
        return;
    }
    
    int window_id = atoi(id_str);
    int line = atoi(line_str);
    
    // Find window
    Window* win = NULL;
    for (int i = 0; i < wm.window_count; i++) {
        if (wm.windows[i].id == window_id) {
            win = &wm.windows[i];
            break;
        }
    }
    
    if (!win) {
        printf("Window %d not found\n", window_id);
        return;
    }
    
    window_write(win, line, text);
    render_screen();
    printf("Text written to window %d, line %d\n", window_id, line);
}

void cmd_exitgui() {
    // Clean up all windows
    for (int i = 0; i < wm.window_count; i++) {
        for (int j = 0; j < wm.windows[i].buffer_lines; j++) {
            free(wm.windows[i].buffer[j]);
        }
        free(wm.windows[i].buffer);
    }
    
    wm.gui_mode = false;
    wm.window_count = 0;
    clear_screen();
    printf("Exited GUI mode. Back to command line.\n");
}

int main() {
    init_fs();
    init_auth();
    wm.gui_mode = false;  // Initialize GUI mode
    
    printf("slopOS v0.1.0 - Secure Operating System Simulator\n");
    printf("=================================================\n\n");
    
    // Login required
    if (!login()) {
        printf("Authentication failed. Exiting.\n");
        return 1;
    }
    
    printf("slopOS simulator with Window Manager. type 'help' for commands.\n");
    char line[256];
    while (true) {
        if (wm.gui_mode) {
            printf("slop-gui@%s$ ", get_current_username());
        } else {
            printf("vibe@%s:%s$ ", get_current_username(), entries[cwd].name);
        }
        
        if (!fgets(line, sizeof(line), stdin)) break;
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        char *cmd = strtok(line, " ");
        if (!cmd) continue;
        
        if (wm.gui_mode) {
            // GUI mode commands
            if (strcmp(cmd, "help") == 0) cmd_help();
            else if (strcmp(cmd, "newwin") == 0) {
                char *args = strtok(NULL, "");
                cmd_newwin(args);
            }
            else if (strcmp(cmd, "closewin") == 0) {
                char *arg = strtok(NULL, " ");
                if (arg) cmd_closewin(atoi(arg));
                else printf("closewin: missing window ID\n");
            }
            else if (strcmp(cmd, "focuswin") == 0) {
                char *arg = strtok(NULL, " ");
                if (arg) cmd_focuswin(atoi(arg));
                else printf("focuswin: missing window ID\n");
            }
            else if (strcmp(cmd, "listwin") == 0) cmd_listwin();
            else if (strcmp(cmd, "writewin") == 0) {
                char *args = strtok(NULL, "");
                cmd_writewin(args);
            }
            else if (strcmp(cmd, "exitgui") == 0) cmd_exitgui();
            else if (strcmp(cmd, "logout") == 0) {
                logout();
                break;
            }
            else printf("Unknown GUI command: %s (type 'help' for commands)\n", cmd);
        } else {
            // Normal filesystem commands
            if (strcmp(cmd, "help") == 0) cmd_help();
            else if (strcmp(cmd, "ls") == 0) cmd_ls();
            else if (strcmp(cmd, "mkdir") == 0) {
                char *arg = strtok(NULL, " ");
                if (arg) cmd_mkdir(arg);
                else printf("mkdir: missing name\n");
            }
            else if (strcmp(cmd, "touch") == 0) {
                char *arg = strtok(NULL, " ");
                if (arg) cmd_touch(arg);
                else printf("touch: missing name\n");
            }
            else if (strcmp(cmd, "cd") == 0) {
                char *arg = strtok(NULL, " ");
                if (arg) cmd_cd(arg);
                else printf("cd: missing name\n");
            }
            else if (strcmp(cmd, "pwd") == 0) cmd_pwd();
            else if (strcmp(cmd, "whoami") == 0) cmd_whoami();
            else if (strcmp(cmd, "adduser") == 0) {
                char *arg = strtok(NULL, " ");
                if (arg) cmd_adduser(arg);
                else printf("adduser: missing username\n");
            }
            else if (strcmp(cmd, "listusers") == 0) cmd_listusers();
            else if (strcmp(cmd, "deluser") == 0) {
                char *arg = strtok(NULL, " ");
                if (arg) cmd_deluser(arg);
                else printf("deluser: missing username\n");
            }
            else if (strcmp(cmd, "passwd") == 0) cmd_passwd();
            else if (strcmp(cmd, "startgui") == 0) cmd_startgui();
            else if (strcmp(cmd, "logout") == 0) {
                logout();
                break;
            }
            else if (strcmp(cmd, "exit") == 0) break;
            else printf("Unknown command: %s\n", cmd);
        }
    }

    // Clean up
    if (wm.gui_mode) {
        for (int i = 0; i < wm.window_count; i++) {
            for (int j = 0; j < wm.windows[i].buffer_lines; j++) {
                free(wm.windows[i].buffer[j]);
            }
            free(wm.windows[i].buffer);
        }
    }
    
    // free filesystem data
    for (int i = 0; i < entry_count; ++i) if (entries[i].data) free(entries[i].data);
    return 0;
}
