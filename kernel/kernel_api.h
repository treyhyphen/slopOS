#ifndef KERNEL_API_H
#define KERNEL_API_H

#include "types.h"

// ====================
// CONSTANTS
// ====================

#define MAX_ENTRIES 100
#define MAX_NAME 32
#define MAX_USERS 16
#define MAX_USERNAME 32
#define MAX_PASSWORD 64
#define ETH_ALEN 6
#define IP_ADDR_LEN 4
#define ARP_CACHE_SIZE 32
#define DNS_CACHE_SIZE 32
#define DNS_MAX_NAME 256
#define MAX_WINDOWS 8
#define RX_BUFFER_SIZE (8192 + 16)
#define TX_BUFFER_SIZE 1536

// VGA Colors
#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW 14
#define VGA_COLOR_WHITE 15

// ====================
// DATA STRUCTURES
// ====================

typedef enum { FILE_ENTRY = 0, DIR_ENTRY = 1 } EntryType;

typedef struct {
    char name[MAX_NAME];      // 32 bytes
    int type;                  // 4 bytes
    int parent;                // 4 bytes  
    int first_child;           // 4 bytes
    int next_sibling;          // 4 bytes
    char content[512];         // 512 bytes
} Entry;  // Total should be 560 bytes

typedef struct {
    char username[MAX_USERNAME];
    char password_hash[MAX_PASSWORD];
    bool is_admin;
    bool active;
} User;

typedef struct {
    User users[MAX_USERS];
    int user_count;
    int current_user;
    bool logged_in;
} AuthSystem;

typedef struct {
    uint8_t ip[IP_ADDR_LEN];
    uint8_t mac[ETH_ALEN];
    bool valid;
} ArpCacheEntry;

// This must match ARPEntry in kernel.c
typedef struct {
    uint8_t ip[IP_ADDR_LEN];
    uint8_t mac[ETH_ALEN];
    bool valid;
} ARPEntry;

typedef struct {
    bool initialized;
    uint16_t io_base;
    uint8_t mac[ETH_ALEN];
    uint8_t ip[IP_ADDR_LEN];
    uint8_t gateway[IP_ADDR_LEN];
    uint8_t netmask[IP_ADDR_LEN];
    uint8_t dns[IP_ADDR_LEN];
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    uint8_t tx_buffer[TX_BUFFER_SIZE];
    uint16_t rx_index;
    ARPEntry arp_cache[ARP_CACHE_SIZE];
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t rx_errors;
    uint32_t tx_errors;
    bool dhcp_configured;
    uint32_t dhcp_xid;
    uint8_t dhcp_server_ip[IP_ADDR_LEN];
} NetworkDevice;

typedef struct {
    bool active;
    uint8_t target_ip[IP_ADDR_LEN];
    uint8_t target_mac[ETH_ALEN];
    uint16_t ping_id;
    uint16_t ping_seq;
    int pings_sent;
    int pings_received;
    int ping_count;
} PingState;

typedef struct {
    int id;
    bool active;
    int x, y, width, height;
    char title[64];
    char content[1024];
    int content_len;
    bool visible;
    bool focused;
    char buffer[1024];
    int buffer_len;
} Window;

// ====================
// EXTERNAL VARIABLES
// ====================

extern Entry entries[MAX_ENTRIES];
extern int entry_count;
extern int cwd;
extern AuthSystem auth;
extern NetworkDevice net_device;
extern PingState ping_state;
extern Window windows[10];
extern int window_count;
extern int next_window_id;
extern bool gui_mode;
extern volatile bool interrupt_command;

// ====================
// KERNEL API FUNCTIONS
// ====================

// Terminal I/O
void terminal_writestring(const char* data);
void terminal_putchar(char c);
void terminal_setcolor(uint8_t color);
void clear_screen(void);
uint8_t vga_entry_color(uint8_t fg, uint8_t bg);

// String/Conversion utilities
void itoa_helper(int n, char* buf);
size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
void strncpy(char* dest, const char* src, size_t n);
void memcpy(void* dest, const void* src, size_t n);
void memset(void* dest, int val, size_t n);

// Timing
void delay_ms(uint32_t ms);

// Filesystem
int alloc_entry(const char* name, int type, int parent);
int find_child_by_name(int dir_idx, const char* name);
void add_child(int parent_idx, int child_idx);
const char* get_current_username(void);

// User management
bool is_current_user_admin(void);
int find_user(const char* username);
void sha256_simple(const char* input, char* output);
bool verify_password(int user_idx, const char* password);
char* read_line(char* buffer, size_t size);

// Network
void net_poll(void);
bool parse_ip(const char* str, uint8_t* ip);
bool is_local_subnet(uint8_t* ip);
uint8_t* arp_lookup(uint8_t* ip);
void arp_send_request(uint8_t* target_ip);
void send_icmp_echo(uint8_t* dest_ip, uint8_t* dest_mac, uint16_t seq);
void send_icmp_request(uint8_t* dest_ip, uint8_t* dest_mac, uint16_t id, uint16_t seq);
bool dns_resolve(const char* name, uint8_t* ip);
bool dhcp_client_run(void);
uint16_t htons(uint16_t n);
void delay_seconds(uint32_t seconds);

// Network driver
const char* net_driver_get_name(void);

// GUI
void render_gui(void);
void render_all_windows(void);
Window* get_window(int id);

#endif // KERNEL_API_H
