/*
 * slopOS Kernel v0.1.0 - Full Featured Bootable OS
 * Complete implementation with authentication, filesystem, and GUI
 */

#include "types.h"
#include "commands.h"

// Configuration constants
#define MAX_NAME 32
#define MAX_ENTRIES 100
#define MAX_USERS 16
#define MAX_PASSWORD 64
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
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_UP_ARROW 0x48
#define KEY_DOWN_ARROW 0x50
#define KEY_EXTENDED 0xE0

// Special return values for arrow keys
#define SPECIAL_UP_ARROW 0x01
#define SPECIAL_DOWN_ARROW 0x02

// PCI Configuration ports
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// RTL8139 Network Card registers
#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139
#define RTL8139_IDR0 0x00      // MAC address
#define RTL8139_MAR0 0x08      // Multicast filter
#define RTL8139_TXSTATUS0 0x10 // Transmit status
#define RTL8139_TXADDR0 0x20   // Transmit address
#define RTL8139_RBSTART 0x30   // Receive buffer start
#define RTL8139_CR 0x37        // Command register
#define RTL8139_CAPR 0x38      // Current address of packet read
#define RTL8139_IMR 0x3C       // Interrupt mask
#define RTL8139_ISR 0x3E       // Interrupt status
#define RTL8139_RCR 0x44       // Receive config
#define RTL8139_CONFIG1 0x52   // Config register

// Network constants
#define ETH_ALEN 6
#define ETH_FRAME_LEN 1514
#define IP_ADDR_LEN 4
#define ARP_CACHE_SIZE 32
#define RX_BUFFER_SIZE 8192 + 16
#define TX_BUFFER_SIZE 1536

// Ethernet protocol types
#define ETH_P_IP 0x0800
#define ETH_P_ARP 0x0806

// IP protocols
#define IPPROTO_ICMP 1
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

// DHCP constants
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_MAGIC_COOKIE 0x63825363
#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTREPLY 2

// DHCP message types
#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DHCP_NAK 6
#define DHCP_RELEASE 7

// DHCP options
#define DHCP_OPT_SUBNET_MASK 1
#define DHCP_OPT_ROUTER 3
#define DHCP_OPT_DNS 6
#define DHCP_OPT_REQUESTED_IP 50
#define DHCP_OPT_LEASE_TIME 51
#define DHCP_OPT_MESSAGE_TYPE 53
#define DHCP_OPT_SERVER_ID 54
#define DHCP_OPT_PARAM_REQUEST 55
#define DHCP_OPT_END 255

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
typedef enum { FILE_ENTRY = 0, DIR_ENTRY = 1 } EntryType;

// Filesystem entry - MUST match kernel_api.h!
typedef struct {
    char name[MAX_NAME];      // 32 bytes
    int type;                  // 4 bytes
    int parent;                // 4 bytes  
    int first_child;           // 4 bytes
    int next_sibling;          // 4 bytes
    char content[512];         // 512 bytes
} Entry;  // Total: 560 bytes

// User structure (internal, matching kernel_api.h)
typedef struct User {
    char username[MAX_NAME];
    char password_hash[MAX_PASSWORD];
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

// Network structures
typedef struct {
    uint8_t dest[ETH_ALEN];
    uint8_t src[ETH_ALEN];
    uint16_t type;
} __attribute__((packed)) EthernetHeader;

typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_size;
    uint8_t proto_size;
    uint16_t opcode;
    uint8_t sender_mac[ETH_ALEN];
    uint8_t sender_ip[IP_ADDR_LEN];
    uint8_t target_mac[ETH_ALEN];
    uint8_t target_ip[IP_ADDR_LEN];
} __attribute__((packed)) ARPPacket;

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t src_ip[IP_ADDR_LEN];
    uint8_t dest_ip[IP_ADDR_LEN];
} __attribute__((packed)) IPHeader;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed)) ICMPHeader;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) UDPHeader;

// TCP structures
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;  // Upper 4 bits = offset, lower 4 = reserved
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) TCPHeader;

// TCP flags
#define TCP_FIN  0x01
#define TCP_SYN  0x02
#define TCP_RST  0x04
#define TCP_PSH  0x08
#define TCP_ACK  0x10
#define TCP_URG  0x20

// TCP states
typedef enum {
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT
} TCPState;

// TCP connection structure
#define MAX_TCP_CONNECTIONS 8
#define TCP_RECV_BUFFER_SIZE 2048
#define TCP_SEND_BUFFER_SIZE 2048

typedef struct {
    bool active;
    TCPState state;
    uint8_t remote_ip[IP_ADDR_LEN];
    uint16_t local_port;
    uint16_t remote_port;
    
    // Sequence numbers
    uint32_t send_seq;
    uint32_t recv_seq;
    uint32_t send_ack;
    
    // Buffers
    uint8_t recv_buffer[TCP_RECV_BUFFER_SIZE];
    uint16_t recv_len;
    uint8_t send_buffer[TCP_SEND_BUFFER_SIZE];
    uint16_t send_len;
    
    // Flags
    bool listening;
    bool fin_received;
    bool fin_sent;
} TCPConnection;

typedef struct {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t magic_cookie;
} __attribute__((packed)) DHCPMessage;

typedef struct {
    uint8_t ip[IP_ADDR_LEN];
    uint8_t mac[ETH_ALEN];
    bool valid;
} ARPEntry;

// DNS structures
#define DNS_CACHE_SIZE 32
#define DNS_MAX_NAME 256
#define DNS_TYPE_A 1      // IPv4 address
#define DNS_CLASS_IN 1    // Internet class

typedef struct {
    char name[DNS_MAX_NAME];
    uint8_t ip[IP_ADDR_LEN];
    bool valid;
    uint32_t timestamp;  // For TTL/expiry (not implemented yet)
} DNSCacheEntry;

typedef struct __attribute__((packed)) {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;  // Question count
    uint16_t ancount;  // Answer count
    uint16_t nscount;  // Authority count
    uint16_t arcount;  // Additional count
} DNSHeader;

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

typedef struct Window {
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

// Globals
// Global state (exported for command modules)
Entry entries[MAX_ENTRIES];
int entry_count = 0;
int cwd = 0;
AuthSystem auth = {0};
Window windows[MAX_WINDOWS];
int window_count = 0;
bool gui_mode = false;
int next_window_id = 1;
NetworkDevice net_device = {0};
PingState ping_state = {0};
volatile bool interrupt_command = false;  // Set when CTRL+C is pressed
static DNSCacheEntry dns_cache[DNS_CACHE_SIZE];
static TCPConnection tcp_connections[MAX_TCP_CONNECTIONS];
static uint32_t tcp_isn = 12345;  // Initial sequence number (should be random)
static uint16_t ip_id_counter = 1;  // IP packet ID counter

// Global TCP connection for flop command
static int flop_conn_id = -1;
static uint16_t dns_query_id = 1;
static char dns_current_query[DNS_MAX_NAME];  // Track current query for response handler
static uint16_t dns_query_port = 0;  // Port used for current DNS query

// Ephemeral port allocation (49152-65535 per RFC 6335)
#define EPHEMERAL_PORT_START 49152
#define EPHEMERAL_PORT_END 65535
static uint16_t next_ephemeral_port = EPHEMERAL_PORT_START;

// Port binding table for tracking open ports
#define MAX_PORT_BINDINGS 32
typedef struct {
    uint16_t port;
    bool in_use;
    void* handler;  // Function pointer to handler (optional)
} PortBinding;
static PortBinding port_bindings[MAX_PORT_BINDINGS];

// Terminal state
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

// Input buffer
static char input_buffer[256];
static size_t input_pos = 0;

// Command history
#define MAX_HISTORY 50
#define MAX_CMD_LEN 256
static char command_history[MAX_HISTORY][MAX_CMD_LEN];
static int history_count = 0;

// Forward declarations
void terminal_writestring(const char* data);
void clear_screen(void);
void net_poll(void);
void handle_udp(uint8_t* src_ip, uint8_t* data, uint16_t len);
uint32_t dhcp_gen_xid(void);
void dhcp_send_discover(void);
void itoa_helper(int n, char* buf);
void itoa_hex(int n, char* buf);
void handle_dns_response(uint8_t* data, uint16_t len);
bool dns_resolve(const char* name, uint8_t* ip);
void dns_cache_init(void);

// Network driver integration functions (from net_integration.c)
bool net_driver_init(void);
const char* net_driver_get_name(void);
void net_driver_send_packet(const void* data, uint16_t length);
void net_driver_poll_packets(void);
void net_driver_get_mac_address(uint8_t* mac);
bool net_driver_is_initialized(void);
void net_driver_rx_packet(const uint8_t* data, uint16_t length);

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

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

// ====================
// TIMING FUNCTIONS
// ====================

// PIT (Programmable Interval Timer) ports and constants
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_FREQUENCY 1193182  // PIT oscillates at ~1.19MHz

static uint64_t tsc_freq_khz = 0; // TSC frequency in KHz

// Read Time Stamp Counter (TSC) - standard x86 timing mechanism
static inline uint64_t rdtsc(void) {
    uint32_t low, high;
    __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

// Calibrate TSC frequency using PIT as reference
// This is the standard approach used by Linux and other OSes
void calibrate_cpu_speed(void) {
    // Configure PIT Channel 2 in one-shot mode
    // We'll use it to create a known time interval
    outb(PIT_COMMAND, 0xB0); // Channel 2, mode 0 (interrupt on terminal count)
    
    // Set up for ~50ms delay (59659 ticks at 1.193182 MHz)
    // 50ms * 1193182 Hz = 59659 ticks
    uint16_t pit_ticks = 59659;
    outb(0x42, pit_ticks & 0xFF);        // Low byte to channel 2
    outb(0x42, (pit_ticks >> 8) & 0xFF); // High byte to channel 2
    
    // Start the PIT counter and read TSC at the same time
    uint8_t pit_control = inb(0x61);
    outb(0x61, (pit_control & 0xFD) | 0x01); // Gate on, speaker off
    
    uint64_t tsc_start = rdtsc();
    
    // Wait for PIT to count down (poll bit 5 of port 0x61)
    // Bit 5 goes high when counter reaches 0
    while ((inb(0x61) & 0x20) == 0);
    
    uint64_t tsc_end = rdtsc();
    
    // Calculate TSC frequency
    // We measured TSC ticks over 50ms = 0.05 seconds
    uint64_t tsc_ticks = tsc_end - tsc_start;
    
    // TSC freq in KHz = (tsc_ticks / 0.05) / 1000 = tsc_ticks / 50
    // Do this without 64-bit division by using shifts
    // Divide by 50: multiply by fixed-point approximation
    // 1/50 ≈ 0.02 = 20971 / 1048576 (shifted by 20 bits)
    // So tsc_ticks / 50 ≈ (tsc_ticks * 20972) >> 20
    uint32_t tsc_low = (uint32_t)(tsc_ticks & 0xFFFFFFFF);
    uint32_t tsc_high = (uint32_t)(tsc_ticks >> 32);
    
    // For simplicity, assume tsc_high is small (typical for 50ms)
    // Approximate: tsc_freq_khz = tsc_ticks / 50
    // Use simple iterative subtraction for division
    tsc_freq_khz = 0;
    while (tsc_ticks >= 50) {
        tsc_ticks -= 50;
        tsc_freq_khz++;
    }
    
    // Sanity check - typical CPU frequencies are 500 MHz to 5 GHz
    // That's 500,000 KHz to 5,000,000 KHz
    if (tsc_freq_khz < 100000 || tsc_freq_khz > 10000000) {
        // Fallback to a reasonable default (2 GHz)
        tsc_freq_khz = 2000000; // 2 GHz in KHz
    }
}

// Delay for specified milliseconds using TSC
void delay_ms(uint32_t ms) {
    if (tsc_freq_khz == 0) return;
    
    uint64_t start = rdtsc();
    uint64_t ticks_per_ms = tsc_freq_khz; // Already in KHz (ticks per ms)
    
    // Calculate end time by adding (ms * ticks_per_ms) to start
    // Do this one ms at a time to avoid 64-bit multiplication
    uint64_t target = start;
    for (uint32_t i = 0; i < ms; i++) {
        target += ticks_per_ms;
    }
    
    while (rdtsc() < target) {
        __asm__ volatile ("pause"); // Hint to CPU we're spinning
    }
}

// Delay for specified seconds
void delay_seconds(uint32_t seconds) {
    delay_ms(seconds * 1000);
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

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    
    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dest;
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

uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | (uint16_t)color << 8;
}

// Update hardware cursor position
void update_cursor(void) {
    uint16_t pos = terminal_row * VGA_WIDTH + terminal_column;
    
    // Cursor LOW port to VGA INDEX register
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    
    // Cursor HIGH port to VGA INDEX register
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
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
    update_cursor();
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll(void) {
    // Normal scrolling - shift everything up one line
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    // Clear the bottom line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    }
    terminal_row = VGA_HEIGHT - 1;
    
    // In GUI mode, redraw windows after scrolling so they stay visible
    if (gui_mode) {
        render_all_windows();
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_scroll();
        update_cursor();
        return;
    }
    
    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        }
        update_cursor();
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_scroll();
    }
    update_cursor();
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
// GUI RENDERING
// ====================

void draw_window(Window* win) {
    if (!win->visible) return;
    
    // Use highly visible colors - white text on blue background for focused
    uint8_t win_color = win->focused ? 
        vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE) :
        vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_CYAN);
    
    // Draw top border with title using solid characters
    for (int x = 0; x < win->width && (win->x + x) < VGA_WIDTH; x++) {
        char c = '=';  // Use = for top border
        if (x > 0 && x < win->width - 1 && x - 1 < (int)strlen(win->title)) {
            c = win->title[x - 1];
        }
        if (win->y < VGA_HEIGHT && (win->x + x) < VGA_WIDTH) {
            terminal_putentryat(c, win_color, win->x + x, win->y);
        }
    }
    
    // Draw sides and content
    for (int y = 1; y < win->height - 1; y++) {
        if ((win->y + y) >= VGA_HEIGHT) break;
        
        // Left border
        if (win->x < VGA_WIDTH) {
            terminal_putentryat('|', win_color, win->x, win->y + y);
        }
        
        // Content area
        for (int x = 1; x < win->width - 1; x++) {
            if ((win->x + x) >= VGA_WIDTH) break;
            terminal_putentryat(' ', win_color, win->x + x, win->y + y);
        }
        
        // Right border
        if ((win->x + win->width - 1) < VGA_WIDTH) {
            terminal_putentryat('|', win_color, win->x + win->width - 1, win->y + y);
        }
    }
    
    // Draw bottom border
    int bottom_y = win->y + win->height - 1;
    if (bottom_y < VGA_HEIGHT) {
        for (int x = 0; x < win->width && (win->x + x) < VGA_WIDTH; x++) {
            char c = '=';  // Use = for bottom border
            terminal_putentryat(c, win_color, win->x + x, bottom_y);
        }
    }
    
    // Draw buffer content inside window
    if (win->buffer_len > 0) {
        int content_x = win->x + 2;
        int content_y = win->y + 1;
        for (int i = 0; i < win->buffer_len && i < (win->width - 4); i++) {
            if (content_x + i < VGA_WIDTH && content_y < VGA_HEIGHT) {
                terminal_putentryat(win->buffer[i], win_color, content_x + i, content_y);
            }
        }
    }
}

void render_all_windows(void) {
    for (int i = 0; i < window_count; i++) {
        draw_window(&windows[i]);
    }
}

// ====================
// KEYBOARD DRIVER
// ====================

// Keyboard scancodes
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_LCTRL 0x1D
#define KEY_C 0x2E

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

// Track shift key state and extended scancode
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool extended = false;

char keyboard_getchar(void) {
    uint8_t scancode;
    
    // Wait for keyboard data
    while (!(inb(KEYBOARD_STATUS_PORT) & 0x01));
    
    scancode = inb(KEYBOARD_DATA_PORT);
    
    // Handle extended keys (0xE0 prefix)
    if (scancode == KEY_EXTENDED) {
        extended = true;
        return 0;
    }
    
    // Handle arrow keys (extended scancodes)
    if (extended) {
        extended = false;
        if (scancode == KEY_UP_ARROW) {
            return SPECIAL_UP_ARROW;
        }
        if (scancode == KEY_DOWN_ARROW) {
            return SPECIAL_DOWN_ARROW;
        }
        return 0;
    }
    
    // Handle CTRL key press
    if (scancode == KEY_LCTRL) {
        ctrl_pressed = true;
        return 0;
    }
    
    // Handle CTRL key release
    if (scancode == (KEY_LCTRL | 0x80)) {
        ctrl_pressed = false;
        return 0;
    }
    
    // Handle CTRL+C interrupt
    if (ctrl_pressed && scancode == KEY_C) {
        interrupt_command = true;
        terminal_writestring("^C\n");
        return 0;
    }
    
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

// Forward declarations for TCP functions
bool tcp_is_established(int conn_id);
int tcp_send_data(int conn_id, uint8_t* data, uint16_t len);
void tcp_close(int conn_id);
int tcp_receive_data(int conn_id, uint8_t* buffer, uint16_t max_len);
void handle_tcp(uint8_t* src_ip, uint8_t* data, uint16_t len);
void send_ip_packet(uint8_t* dest_ip, uint8_t* packet, uint16_t len);
uint16_t allocate_ephemeral_port(void);
void free_ephemeral_port(uint16_t port);

void read_line(char* buffer, size_t max_len) {
    size_t pos = 0;
    int current_history_pos = history_count;  // Start at end of history
    
    while (1) {
        // Check for CTRL+C interrupt
        if (interrupt_command) {
            interrupt_command = false;
            
            // If flop is active, close the connection
            if (flop_conn_id >= 0) {
                terminal_writestring("closing connection...\n");
                tcp_close(flop_conn_id);
                flop_conn_id = -1;
            }
            
            buffer[0] = '\0';
            return;
        }
        
        char c = keyboard_getchar();
        
        if (c == 0) continue;
        
        // Handle up arrow - go back in history
        if (c == SPECIAL_UP_ARROW) {
            if (current_history_pos > 0) {
                current_history_pos--;
                
                // Clear current line
                while (pos > 0) {
                    terminal_putchar('\b');
                    pos--;
                }
                
                // Copy history entry to buffer
                size_t i = 0;
                while (command_history[current_history_pos][i] && i < max_len - 1) {
                    buffer[i] = command_history[current_history_pos][i];
                    i++;
                }
                buffer[i] = '\0';
                pos = i;
                
                // Display it
                for (size_t j = 0; j < pos; j++) {
                    terminal_putchar(buffer[j]);
                }
            }
            continue;
        }
        
        // Handle down arrow - go forward in history
        if (c == SPECIAL_DOWN_ARROW) {
            if (current_history_pos < history_count) {
                current_history_pos++;
                
                // Clear current line
                while (pos > 0) {
                    terminal_putchar('\b');
                    pos--;
                }
                
                // If at end of history, show blank line
                if (current_history_pos == history_count) {
                    buffer[0] = '\0';
                    pos = 0;
                } else {
                    // Copy history entry to buffer
                    size_t i = 0;
                    while (command_history[current_history_pos][i] && i < max_len - 1) {
                        buffer[i] = command_history[current_history_pos][i];
                        i++;
                    }
                    buffer[i] = '\0';
                    pos = i;
                    
                    // Display it
                    for (size_t j = 0; j < pos; j++) {
                        terminal_putchar(buffer[j]);
                    }
                }
            }
            continue;
        }
        
        if (c == '\n') {
            buffer[pos] = '\0';
            terminal_putchar('\n');
            
            // Add to history if non-empty and different from last command
            if (pos > 0 && (history_count == 0 || strcmp(buffer, command_history[history_count - 1]) != 0)) {
                if (history_count < MAX_HISTORY) {
                    // Copy to next history slot
                    size_t i = 0;
                    while (buffer[i] && i < MAX_CMD_LEN - 1) {
                        command_history[history_count][i] = buffer[i];
                        i++;
                    }
                    command_history[history_count][i] = '\0';
                    history_count++;
                } else {
                    // Shift history up and add new command at end
                    for (int i = 0; i < MAX_HISTORY - 1; i++) {
                        size_t j = 0;
                        while (command_history[i + 1][j] && j < MAX_CMD_LEN - 1) {
                            command_history[i][j] = command_history[i + 1][j];
                            j++;
                        }
                        command_history[i][j] = '\0';
                    }
                    // Add new command at end
                    size_t i = 0;
                    while (buffer[i] && i < MAX_CMD_LEN - 1) {
                        command_history[MAX_HISTORY - 1][i] = buffer[i];
                        i++;
                    }
                    command_history[MAX_HISTORY - 1][i] = '\0';
                }
            }
            
            return;
        }
        
        if (c == '\b') {
            if (pos > 0) {
                pos--;
                terminal_putchar('\b');
            }
        } else if (pos < max_len - 1 && c >= ' ') {  // Only accept printable characters
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
// NETWORK FUNCTIONS
// ====================

// Byte order conversion
uint16_t htons(uint16_t hostshort) {
    return (hostshort >> 8) | (hostshort << 8);
}

uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);
}

uint32_t htonl(uint32_t hostlong) {
    return ((hostlong >> 24) & 0x000000FF) |
           ((hostlong >> 8) & 0x0000FF00) |
           ((hostlong << 8) & 0x00FF0000) |
           ((hostlong << 24) & 0xFF000000);
}

uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong);
}

// PCI configuration space access
uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | 
                                  (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | 
                                  (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

// ========================================
// OLD RTL8139-SPECIFIC CODE (NOT USED ANYMORE)
// Now using driver abstraction layer
// ========================================

/*
// Find RTL8139 network card
bool find_rtl8139(void) {
    for (uint8_t bus = 0; bus < 8; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vendor_device = pci_config_read(bus, slot, 0, 0);
            uint16_t vendor = vendor_device & 0xFFFF;
            uint16_t device = (vendor_device >> 16) & 0xFFFF;
            
            if (vendor == RTL8139_VENDOR_ID && device == RTL8139_DEVICE_ID) {
                // Found RTL8139! Get IO base address
                uint32_t bar0 = pci_config_read(bus, slot, 0, 0x10);
                net_device.io_base = bar0 & ~0x3;
                
                // Enable bus mastering
                uint32_t command = pci_config_read(bus, slot, 0, 0x04);
                pci_config_write(bus, slot, 0, 0x04, command | 0x05);
                
                return true;
            }
        }
    }
    return false;
}

// RTL8139 initialization
void rtl8139_init(void) {
    if (!find_rtl8139()) {
        net_device.initialized = false;
        return;
    }
    
    uint16_t io = net_device.io_base;
    
    // Power on
    outb(io + RTL8139_CONFIG1, 0x00);
    
    // Software reset
    outb(io + RTL8139_CR, 0x10);
    while (inb(io + RTL8139_CR) & 0x10);
    
    // Init receive buffer
    outl(io + RTL8139_RBSTART, (uint32_t)net_device.rx_buffer);
    
    // Enable transmit and receive
    outb(io + RTL8139_CR, 0x0C);
    
    // Configure receive: accept broadcast, multicast, physical match
    outl(io + RTL8139_RCR, 0x0F);
    
    // Read MAC address
    for (int i = 0; i < ETH_ALEN; i++) {
        net_device.mac[i] = inb(io + RTL8139_IDR0 + i);
    }
    
    // Initialize IP to 0.0.0.0 (will be set by DHCP)
    memset(net_device.ip, 0, IP_ADDR_LEN);
    memset(net_device.gateway, 0, IP_ADDR_LEN);
    memset(net_device.netmask, 0, IP_ADDR_LEN);
    memset(net_device.dns, 0, IP_ADDR_LEN);
    
    net_device.rx_index = 0;
    net_device.dhcp_configured = false;
    net_device.initialized = true;
}
*/

// ========================================
// END OF OLD RTL8139 CODE
// ========================================


// Run DHCP client to get IP address
bool dhcp_client_run(void) {
    if (!net_device.initialized) return false;
    
    // Generate transaction ID
    net_device.dhcp_xid = dhcp_gen_xid();
    
    // Try up to 3 times to get DHCP configuration
    for (int retry = 0; retry < 3; retry++) {
        if (retry > 0) {
            terminal_writestring("  DHCP: Retry attempt ");
            char buf[4];
            itoa_helper(retry + 1, buf);
            terminal_writestring(buf);
            terminal_writestring("...\n");
            delay_seconds(2); // Wait 2 seconds between retries
        }
        
        // Send DISCOVER
        terminal_writestring("  DHCP: Sending DISCOVER...\n");
        dhcp_send_discover();
        
        // Wait for OFFER (30 seconds = 3000 polls with 10ms delay each)
        terminal_writestring("  DHCP: Waiting for OFFER");
        int wait_attempts = 0;
        bool offer_received = false;
        while (wait_attempts < 3000 && !net_device.dhcp_configured) {
            if (interrupt_command) {
                terminal_writestring("\nInterrupted\n");
                return false;
            }
            net_poll();
            
            // Check if we somehow got fully configured already (fast server)
            if (net_device.dhcp_configured) {
                terminal_writestring("\n  DHCP: Configuration complete!\n");
                return true;
            }
            
            // Check if we got an OFFER (REQUEST would have been sent)
            // We can detect this by checking if we have a server IP set
            if (net_device.dhcp_server_ip[0] != 0) {
                offer_received = true;
                terminal_writestring("\n  DHCP: OFFER received, sending REQUEST...\n");
                break;
            }
            
            // Show progress dots every 100 attempts (~1 second)
            if (wait_attempts > 0 && wait_attempts % 100 == 0) {
                terminal_putchar('.');
            }
            
            delay_ms(10);
            wait_attempts++;
        }
        
        if (!offer_received) {
            terminal_writestring("\n  DHCP: No OFFER received (timeout after 30s)\n");
            continue; // Try again
        }
        
        // Wait for ACK (30 seconds = 3000 polls with 10ms delay each)
        terminal_writestring("  DHCP: Waiting for ACK");
        wait_attempts = 0;
        while (wait_attempts < 3000 && !net_device.dhcp_configured) {
            if (interrupt_command) {
                terminal_writestring("\nInterrupted\n");
                return false;
            }
            net_poll();
            
            if (net_device.dhcp_configured) {
                terminal_writestring("\n");
                // Success! Do a final poll burst to ensure all packets processed
                for (int i = 0; i < 20; i++) {
                    net_poll();
                    delay_ms(10);
                }
                return true;
            }
            
            // Show progress dots every 100 attempts (~1 second)
            if (wait_attempts > 0 && wait_attempts % 100 == 0) {
                terminal_putchar('.');
            }
            
            delay_ms(10);
            wait_attempts++;
        }
        
        if (!net_device.dhcp_configured) {
            terminal_writestring("\n  DHCP: No ACK received (timeout after 30s)\n");
            // Reset server IP for next retry
            net_device.dhcp_server_ip[0] = 0;
        }
    }
    
    return net_device.dhcp_configured;
}

// Calculate IP checksum
uint16_t ip_checksum(void* data, int len) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)data;
    
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    
    if (len > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Send ethernet frame
void net_send(void* data, uint16_t len) {
    if (!net_driver_is_initialized() || len > TX_BUFFER_SIZE) {
        net_device.tx_errors++;
        return;
    }
    
    // Use driver to send packet
    net_driver_send_packet(data, len);
    net_device.tx_packets++;
}

// ARP cache lookup
uint8_t* arp_lookup(uint8_t* ip) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (net_device.arp_cache[i].valid &&
            memcmp(net_device.arp_cache[i].ip, ip, IP_ADDR_LEN) == 0) {
            return net_device.arp_cache[i].mac;
        }
    }
    return NULL;
}

// Add entry to ARP cache
void arp_cache_add(uint8_t* ip, uint8_t* mac) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!net_device.arp_cache[i].valid) {
            for (int j = 0; j < IP_ADDR_LEN; j++)
                net_device.arp_cache[i].ip[j] = ip[j];
            for (int j = 0; j < ETH_ALEN; j++)
                net_device.arp_cache[i].mac[j] = mac[j];
            net_device.arp_cache[i].valid = true;
            return;
        }
    }
}

// ====================
// ROUTING FUNCTIONS
// ====================

// Check if IP is on local subnet
bool is_local_subnet(uint8_t* ip) {
    // If no netmask configured, assume local
    if (net_device.netmask[0] == 0) return true;
    
    // Compare IP with our IP under netmask
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        if ((ip[i] & net_device.netmask[i]) != (net_device.ip[i] & net_device.netmask[i])) {
            return false; // Different subnet
        }
    }
    return true; // Same subnet
}

// Get next hop MAC for routing (returns MAC to send packet to)
// For local addresses: returns MAC of destination
// For remote addresses: returns MAC of gateway
uint8_t* route_get_next_hop_mac(uint8_t* dest_ip) {
    // Check if destination is on local subnet
    if (is_local_subnet(dest_ip)) {
        // Local destination - ARP for the destination itself
        return arp_lookup(dest_ip);
    } else {
        // Remote destination - ARP for the gateway
        if (net_device.gateway[0] == 0) {
            // No gateway configured
            return NULL;
        }
        return arp_lookup(net_device.gateway);
    }
}

// Send ARP request
void arp_send_request(uint8_t* target_ip) {
    uint8_t frame[sizeof(EthernetHeader) + sizeof(ARPPacket)];
    EthernetHeader* eth = (EthernetHeader*)frame;
    ARPPacket* arp = (ARPPacket*)(frame + sizeof(EthernetHeader));
    
    // Ethernet header
    memset(eth->dest, 0xFF, ETH_ALEN); // Broadcast
    for (int i = 0; i < ETH_ALEN; i++)
        eth->src[i] = net_device.mac[i];
    eth->type = htons(ETH_P_ARP);
    
    // ARP packet
    arp->hw_type = htons(1); // Ethernet
    arp->proto_type = htons(ETH_P_IP);
    arp->hw_size = ETH_ALEN;
    arp->proto_size = IP_ADDR_LEN;
    arp->opcode = htons(1); // Request
    
    for (int i = 0; i < ETH_ALEN; i++)
        arp->sender_mac[i] = net_device.mac[i];
    for (int i = 0; i < IP_ADDR_LEN; i++)
        arp->sender_ip[i] = net_device.ip[i];
    
    memset(arp->target_mac, 0, ETH_ALEN);
    for (int i = 0; i < IP_ADDR_LEN; i++)
        arp->target_ip[i] = target_ip[i];
    
    net_send(frame, sizeof(frame));
}

// Process ARP packet
void handle_arp(uint8_t* data, uint16_t len) {
    if (len < sizeof(ARPPacket)) return;
    
    ARPPacket* arp = (ARPPacket*)data;
    uint16_t opcode = ntohs(arp->opcode);
    
    // Add to cache
    arp_cache_add(arp->sender_ip, arp->sender_mac);
    
    // If it's a request for us, send reply
    if (opcode == 1 && memcmp(arp->target_ip, net_device.ip, IP_ADDR_LEN) == 0) {
        uint8_t frame[sizeof(EthernetHeader) + sizeof(ARPPacket)];
        EthernetHeader* eth = (EthernetHeader*)frame;
        ARPPacket* reply = (ARPPacket*)(frame + sizeof(EthernetHeader));
        
        // Ethernet header
        for (int i = 0; i < ETH_ALEN; i++) {
            eth->dest[i] = arp->sender_mac[i];
            eth->src[i] = net_device.mac[i];
        }
        eth->type = htons(ETH_P_ARP);
        
        // ARP reply
        reply->hw_type = htons(1);
        reply->proto_type = htons(ETH_P_IP);
        reply->hw_size = ETH_ALEN;
        reply->proto_size = IP_ADDR_LEN;
        reply->opcode = htons(2); // Reply
        
        for (int i = 0; i < ETH_ALEN; i++)
            reply->sender_mac[i] = net_device.mac[i];
        for (int i = 0; i < IP_ADDR_LEN; i++)
            reply->sender_ip[i] = net_device.ip[i];
        for (int i = 0; i < ETH_ALEN; i++)
            reply->target_mac[i] = arp->sender_mac[i];
        for (int i = 0; i < IP_ADDR_LEN; i++)
            reply->target_ip[i] = arp->sender_ip[i];
        
        net_send(frame, sizeof(frame));
    }
}

// Send ICMP echo reply
void send_icmp_reply(uint8_t* dest_ip, uint8_t* dest_mac, uint16_t id, uint16_t seq) {
    uint8_t frame[sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(ICMPHeader)];
    EthernetHeader* eth = (EthernetHeader*)frame;
    IPHeader* ip = (IPHeader*)(frame + sizeof(EthernetHeader));
    ICMPHeader* icmp = (ICMPHeader*)(frame + sizeof(EthernetHeader) + sizeof(IPHeader));
    
    // Ethernet header
    for (int i = 0; i < ETH_ALEN; i++) {
        eth->dest[i] = dest_mac[i];
        eth->src[i] = net_device.mac[i];
    }
    eth->type = htons(ETH_P_IP);
    
    // IP header
    ip->version_ihl = 0x45; // IPv4, 20 bytes header
    ip->tos = 0;
    ip->total_length = htons(sizeof(IPHeader) + sizeof(ICMPHeader));
    ip->id = 0;
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_ICMP;
    ip->checksum = 0;
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        ip->src_ip[i] = net_device.ip[i];
        ip->dest_ip[i] = dest_ip[i];
    }
    ip->checksum = ip_checksum(ip, sizeof(IPHeader));
    
    // ICMP header
    icmp->type = 0; // Echo reply
    icmp->code = 0;
    icmp->id = id;
    icmp->sequence = seq;
    icmp->checksum = 0;
    icmp->checksum = ip_checksum(icmp, sizeof(ICMPHeader));
    
    net_send(frame, sizeof(frame));
}

// Send ICMP echo request (ping)
void send_icmp_request(uint8_t* dest_ip, uint8_t* dest_mac, uint16_t id, uint16_t seq) {
    uint8_t frame[sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(ICMPHeader)];
    EthernetHeader* eth = (EthernetHeader*)frame;
    IPHeader* ip = (IPHeader*)(frame + sizeof(EthernetHeader));
    ICMPHeader* icmp = (ICMPHeader*)(frame + sizeof(EthernetHeader) + sizeof(IPHeader));
    
    // Ethernet header
    for (int i = 0; i < ETH_ALEN; i++) {
        eth->dest[i] = dest_mac[i];
        eth->src[i] = net_device.mac[i];
    }
    eth->type = htons(ETH_P_IP);
    
    // IP header
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = htons(sizeof(IPHeader) + sizeof(ICMPHeader));
    ip->id = 0;
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_ICMP;
    ip->checksum = 0;
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        ip->src_ip[i] = net_device.ip[i];
        ip->dest_ip[i] = dest_ip[i];
    }
    ip->checksum = ip_checksum(ip, sizeof(IPHeader));
    
    // ICMP header - Echo Request (type 8)
    icmp->type = 8; // Echo request
    icmp->code = 0;
    icmp->id = id;
    icmp->sequence = seq;
    icmp->checksum = 0;
    icmp->checksum = ip_checksum(icmp, sizeof(ICMPHeader));
    
    net_send(frame, sizeof(frame));
}

// Wrapper for network_commands.c compatibility
void send_icmp_echo(uint8_t* dest_ip, uint8_t* dest_mac, uint16_t seq) {
    send_icmp_request(dest_ip, dest_mac, 0x1234, htons(seq));
}

// Process ICMP packet
void handle_icmp(uint8_t* src_ip, uint8_t* src_mac, uint8_t* data, uint16_t len) {
    if (len < sizeof(ICMPHeader)) return;
    
    ICMPHeader* icmp = (ICMPHeader*)data;
    
    // If it's an echo request (ping), send reply
    if (icmp->type == 8 && icmp->code == 0) {
        send_icmp_reply(src_ip, src_mac, icmp->id, icmp->sequence);
    }
    // If it's an echo reply and we're pinging
    else if (icmp->type == 0 && icmp->code == 0 && ping_state.active) {
        if (icmp->id == ping_state.ping_id && icmp->sequence == ping_state.ping_seq) {
            ping_state.pings_received++;
            
            terminal_writestring("  Reply from ");
            for (int i = 0; i < IP_ADDR_LEN; i++) {
                char buf[16];
                itoa_helper(src_ip[i], buf);
                terminal_writestring(buf);
                if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
            }
            terminal_writestring(" seq=");
            char buf[16];
            itoa_helper(ntohs(icmp->sequence), buf);
            terminal_writestring(buf);
            terminal_writestring("\n");
        }
    }
}

// Process IP packet
void handle_ip(uint8_t* src_mac, uint8_t* data, uint16_t len) {
    if (len < sizeof(IPHeader)) return;
    
    IPHeader* ip = (IPHeader*)data;
    
    // Check if it's for us or broadcast
    bool for_us = (memcmp(ip->dest_ip, net_device.ip, IP_ADDR_LEN) == 0);
    bool broadcast = (ip->dest_ip[0] == 255 && ip->dest_ip[1] == 255 && 
                      ip->dest_ip[2] == 255 && ip->dest_ip[3] == 255);
    
    if (!for_us && !broadcast) return;
    
    uint8_t protocol = ip->protocol;
    uint8_t* payload = data + sizeof(IPHeader);
    uint16_t payload_len = len - sizeof(IPHeader);
    
    if (protocol == IPPROTO_ICMP) {
        handle_icmp(ip->src_ip, src_mac, payload, payload_len);
    } else if (protocol == IPPROTO_UDP) {
        handle_udp(ip->src_ip, payload, payload_len);
    } else if (protocol == 6) {  // TCP
        handle_tcp(ip->src_ip, payload, payload_len);
    }
}

// Process received ethernet frame
void handle_ethernet_frame(uint8_t* data, uint16_t len) {
    if (len < sizeof(EthernetHeader)) return;
    
    EthernetHeader* eth = (EthernetHeader*)data;
    uint16_t eth_type = ntohs(eth->type);
    uint8_t* payload = data + sizeof(EthernetHeader);
    uint16_t payload_len = len - sizeof(EthernetHeader);
    
    if (eth_type == ETH_P_ARP) {
        handle_arp(payload, payload_len);
    } else if (eth_type == ETH_P_IP) {
        handle_ip(eth->src, payload, payload_len);
    }
}

// Poll for received packets
void net_poll(void) {
    if (!net_driver_is_initialized()) return;
    
    // Use driver to poll for packets
    // Received packets will be delivered via net_driver_rx_packet callback
    net_driver_poll_packets();
}

// Callback from network driver when packet is received
void net_driver_rx_packet(const uint8_t* data, uint16_t length) {
    if (length > 0 && length < ETH_FRAME_LEN) {
        // Process the ethernet frame
        handle_ethernet_frame((uint8_t*)data, length);
        net_device.rx_packets++;
    } else {
        net_device.rx_errors++;
    }
}

// UDP checksum (using pseudo-header)
uint16_t udp_checksum(uint8_t* src_ip, uint8_t* dest_ip, UDPHeader* udp, uint8_t* data, uint16_t data_len) {
    uint32_t sum = 0;
    
    // Pseudo-header
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        sum += src_ip[i] << ((i & 1) ? 0 : 8);
        sum += dest_ip[i] << ((i & 1) ? 0 : 8);
    }
    sum += IPPROTO_UDP;
    sum += udp->length;
    
    // UDP header
    uint16_t* ptr = (uint16_t*)udp;
    sum += ptr[0]; // src_port
    sum += ptr[1]; // dest_port
    sum += ptr[2]; // length
    // skip checksum field
    
    // Data
    ptr = (uint16_t*)data;
    int len = data_len;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Send UDP packet
void send_udp(uint8_t* dest_ip, uint8_t* dest_mac, uint16_t src_port, uint16_t dest_port, 
              uint8_t* data, uint16_t data_len) {
    uint16_t total_len = sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(UDPHeader) + data_len;
    if (total_len > TX_BUFFER_SIZE) return;
    
    uint8_t frame[TX_BUFFER_SIZE];
    EthernetHeader* eth = (EthernetHeader*)frame;
    IPHeader* ip = (IPHeader*)(frame + sizeof(EthernetHeader));
    UDPHeader* udp = (UDPHeader*)(frame + sizeof(EthernetHeader) + sizeof(IPHeader));
    uint8_t* payload = frame + sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(UDPHeader);
    
    // Ethernet header
    for (int i = 0; i < ETH_ALEN; i++) {
        eth->dest[i] = dest_mac[i];
        eth->src[i] = net_device.mac[i];
    }
    eth->type = htons(ETH_P_IP);
    
    // IP header
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = htons(sizeof(IPHeader) + sizeof(UDPHeader) + data_len);
    ip->id = 0;
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_UDP;
    ip->checksum = 0;
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        ip->src_ip[i] = net_device.ip[i];
        ip->dest_ip[i] = dest_ip[i];
    }
    ip->checksum = ip_checksum(ip, sizeof(IPHeader));
    
    // UDP header
    udp->src_port = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons(sizeof(UDPHeader) + data_len);
    udp->checksum = 0;
    
    // Copy data
    for (uint16_t i = 0; i < data_len; i++) {
        payload[i] = data[i];
    }
    
    // Calculate UDP checksum (optional, can leave as 0)
    udp->checksum = 0; // UDP checksum is optional for IPv4
    
    net_send(frame, total_len);
}

// Process UDP packet
void handle_udp(uint8_t* src_ip, uint8_t* data, uint16_t len);

// Generate random XID for DHCP
uint32_t dhcp_gen_xid(void) {
    static uint32_t xid = 0x12345678;
    xid = xid * 1103515245 + 12345;
    return xid;
}

// Send DHCP DISCOVER
void dhcp_send_discover(void) {
    uint8_t buffer[sizeof(DHCPMessage) + 64];
    DHCPMessage* dhcp = (DHCPMessage*)buffer;
    
    memset(dhcp, 0, sizeof(DHCPMessage));
    
    dhcp->op = DHCP_BOOTREQUEST;
    dhcp->htype = 1; // Ethernet
    dhcp->hlen = ETH_ALEN;
    dhcp->hops = 0;
    dhcp->xid = htonl(net_device.dhcp_xid);
    dhcp->secs = 0;
    dhcp->flags = htons(0x8000); // Broadcast flag
    
    for (int i = 0; i < ETH_ALEN; i++) {
        dhcp->chaddr[i] = net_device.mac[i];
    }
    
    dhcp->magic_cookie = htonl(DHCP_MAGIC_COOKIE);
    
    // Add options
    uint8_t* opts = buffer + sizeof(DHCPMessage);
    int opt_len = 0;
    
    // Option 53: DHCP Message Type = DISCOVER
    opts[opt_len++] = DHCP_OPT_MESSAGE_TYPE;
    opts[opt_len++] = 1;
    opts[opt_len++] = DHCP_DISCOVER;
    
    // Option 55: Parameter Request List
    opts[opt_len++] = DHCP_OPT_PARAM_REQUEST;
    opts[opt_len++] = 3;
    opts[opt_len++] = DHCP_OPT_SUBNET_MASK;
    opts[opt_len++] = DHCP_OPT_ROUTER;
    opts[opt_len++] = DHCP_OPT_DNS;
    
    // Option 255: End
    opts[opt_len++] = DHCP_OPT_END;
    
    // Send via UDP broadcast
    uint8_t broadcast_ip[IP_ADDR_LEN] = {255, 255, 255, 255};
    uint8_t broadcast_mac[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    send_udp(broadcast_ip, broadcast_mac, DHCP_CLIENT_PORT, DHCP_SERVER_PORT,
             buffer, sizeof(DHCPMessage) + opt_len);
}

// Send DHCP REQUEST
void dhcp_send_request(uint8_t* offered_ip, uint8_t* server_ip) {
    uint8_t buffer[sizeof(DHCPMessage) + 64];
    DHCPMessage* dhcp = (DHCPMessage*)buffer;
    
    memset(dhcp, 0, sizeof(DHCPMessage));
    
    dhcp->op = DHCP_BOOTREQUEST;
    dhcp->htype = 1;
    dhcp->hlen = ETH_ALEN;
    dhcp->hops = 0;
    dhcp->xid = htonl(net_device.dhcp_xid);
    dhcp->secs = 0;
    dhcp->flags = htons(0x8000);
    
    for (int i = 0; i < ETH_ALEN; i++) {
        dhcp->chaddr[i] = net_device.mac[i];
    }
    
    dhcp->magic_cookie = htonl(DHCP_MAGIC_COOKIE);
    
    // Add options
    uint8_t* opts = buffer + sizeof(DHCPMessage);
    int opt_len = 0;
    
    // Option 53: DHCP Message Type = REQUEST
    opts[opt_len++] = DHCP_OPT_MESSAGE_TYPE;
    opts[opt_len++] = 1;
    opts[opt_len++] = DHCP_REQUEST;
    
    // Option 50: Requested IP
    opts[opt_len++] = DHCP_OPT_REQUESTED_IP;
    opts[opt_len++] = 4;
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        opts[opt_len++] = offered_ip[i];
    }
    
    // Option 54: Server Identifier
    opts[opt_len++] = DHCP_OPT_SERVER_ID;
    opts[opt_len++] = 4;
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        opts[opt_len++] = server_ip[i];
    }
    
    // Option 255: End
    opts[opt_len++] = DHCP_OPT_END;
    
    // Send via UDP broadcast
    uint8_t broadcast_ip[IP_ADDR_LEN] = {255, 255, 255, 255};
    uint8_t broadcast_mac[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    send_udp(broadcast_ip, broadcast_mac, DHCP_CLIENT_PORT, DHCP_SERVER_PORT,
             buffer, sizeof(DHCPMessage) + opt_len);
}

// Parse DHCP options
void dhcp_parse_options(uint8_t* opts, int len, uint8_t* msg_type, 
                        uint8_t* server_ip, uint8_t* subnet, uint8_t* router, uint8_t* dns) {
    int i = 0;
    while (i < len) {
        uint8_t opt = opts[i++];
        
        if (opt == DHCP_OPT_END) break;
        if (opt == 0) continue; // Padding
        
        uint8_t opt_len = opts[i++];
        
        if (opt == DHCP_OPT_MESSAGE_TYPE && opt_len == 1) {
            *msg_type = opts[i];
        } else if (opt == DHCP_OPT_SERVER_ID && opt_len == 4) {
            for (int j = 0; j < 4; j++) server_ip[j] = opts[i + j];
        } else if (opt == DHCP_OPT_SUBNET_MASK && opt_len == 4) {
            for (int j = 0; j < 4; j++) subnet[j] = opts[i + j];
        } else if (opt == DHCP_OPT_ROUTER && opt_len >= 4) {
            for (int j = 0; j < 4; j++) router[j] = opts[i + j];
        } else if (opt == DHCP_OPT_DNS && opt_len >= 4) {
            for (int j = 0; j < 4; j++) dns[j] = opts[i + j];
        }
        
        i += opt_len;
    }
}

// Handle DHCP message
void handle_dhcp(uint8_t* data, uint16_t len) {
    if (len < sizeof(DHCPMessage)) return;
    
    DHCPMessage* dhcp = (DHCPMessage*)data;
    
    // Check if it's for us
    if (ntohl(dhcp->xid) != net_device.dhcp_xid) return;
    if (dhcp->op != DHCP_BOOTREPLY) return;
    
    // Parse options
    uint8_t msg_type = 0;
    uint8_t server_ip[IP_ADDR_LEN] = {0};
    uint8_t subnet[IP_ADDR_LEN] = {0};
    uint8_t router[IP_ADDR_LEN] = {0};
    uint8_t dns[IP_ADDR_LEN] = {0};
    
    uint8_t* opts = data + sizeof(DHCPMessage);
    int opts_len = len - sizeof(DHCPMessage);
    
    dhcp_parse_options(opts, opts_len, &msg_type, server_ip, subnet, router, dns);
    
    if (msg_type == DHCP_OFFER) {
        // Extract offered IP
        uint8_t offered_ip[IP_ADDR_LEN];
        uint32_t yiaddr = ntohl(dhcp->yiaddr);
        offered_ip[0] = (yiaddr >> 24) & 0xFF;
        offered_ip[1] = (yiaddr >> 16) & 0xFF;
        offered_ip[2] = (yiaddr >> 8) & 0xFF;
        offered_ip[3] = yiaddr & 0xFF;
        
        // Save server IP
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            net_device.dhcp_server_ip[i] = server_ip[i];
        }
        
        // Send REQUEST
        dhcp_send_request(offered_ip, server_ip);
        terminal_writestring("  DHCP: Received OFFER, sending REQUEST...\n");
        
    } else if (msg_type == DHCP_ACK) {
        // Configuration accepted!
        uint32_t yiaddr = ntohl(dhcp->yiaddr);
        net_device.ip[0] = (yiaddr >> 24) & 0xFF;
        net_device.ip[1] = (yiaddr >> 16) & 0xFF;
        net_device.ip[2] = (yiaddr >> 8) & 0xFF;
        net_device.ip[3] = yiaddr & 0xFF;
        
        // Set netmask
        if (subnet[0] != 0) {
            for (int i = 0; i < IP_ADDR_LEN; i++) {
                net_device.netmask[i] = subnet[i];
            }
        }
        
        // Set gateway
        if (router[0] != 0) {
            for (int i = 0; i < IP_ADDR_LEN; i++) {
                net_device.gateway[i] = router[i];
            }
        }
        
        // Set DNS
        if (dns[0] != 0) {
            for (int i = 0; i < IP_ADDR_LEN; i++) {
                net_device.dns[i] = dns[i];
            }
        }
        
        net_device.dhcp_configured = true;
        terminal_writestring("  DHCP: Received ACK, configuration complete!\n");
    }
}

// Process UDP packet
void handle_udp(uint8_t* src_ip, uint8_t* data, uint16_t len) {
    if (len < sizeof(UDPHeader)) return;
    
    UDPHeader* udp = (UDPHeader*)data;
    uint16_t dest_port = ntohs(udp->dest_port);
    uint16_t src_port = ntohs(udp->src_port);
    
    uint8_t* payload = data + sizeof(UDPHeader);
    uint16_t payload_len = len - sizeof(UDPHeader);
    
    // Debug: Show UDP packet details (comment out after debugging)
    /*
    terminal_writestring("UDP: ");
    char buf[8];
    itoa_helper(src_port, buf);
    terminal_writestring(buf);
    terminal_writestring(" -> ");
    itoa_helper(dest_port, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    */
    
    // Handle DHCP
    if (dest_port == DHCP_CLIENT_PORT && src_port == DHCP_SERVER_PORT) {
        handle_dhcp(payload, payload_len);
    }
    
    // Handle DNS responses (from port 53 to our allocated ephemeral port)
    if (src_port == 53 && dns_query_port != 0 && dest_port == dns_query_port) {
        terminal_writestring("DNS: Received UDP packet from port 53\n");
        handle_dns_response(payload, payload_len);
    }
}

// ====================
// TCP IMPLEMENTATION
// ====================

// Calculate TCP checksum
uint16_t tcp_checksum(uint8_t* src_ip, uint8_t* dest_ip, TCPHeader* tcp, uint8_t* data, uint16_t tcp_len) {
    uint32_t sum = 0;
    
    // Pseudo-header: source IP (4 bytes)
    sum += (src_ip[0] << 8) + src_ip[1];
    sum += (src_ip[2] << 8) + src_ip[3];
    
    // Pseudo-header: destination IP (4 bytes)
    sum += (dest_ip[0] << 8) + dest_ip[1];
    sum += (dest_ip[2] << 8) + dest_ip[3];
    
    // Pseudo-header: zero + protocol (2 bytes)
    sum += 6;  // Protocol = TCP
    
    // Pseudo-header: TCP length (2 bytes)
    sum += tcp_len;
    
    // TCP header + data as 16-bit words
    uint16_t* ptr = (uint16_t*)tcp;
    for (int i = 0; i < tcp_len / 2; i++) {
        sum += ntohs(ptr[i]);
    }
    
    // Handle odd byte
    if (tcp_len % 2) {
        sum += ((uint8_t*)tcp)[tcp_len - 1] << 8;
    }
    
    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Find free TCP connection slot
TCPConnection* tcp_find_free_slot(void) {
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (!tcp_connections[i].active) {
            memset(&tcp_connections[i], 0, sizeof(TCPConnection));
            tcp_connections[i].active = true;
            return &tcp_connections[i];
        }
    }
    return NULL;
}

// Find TCP connection by port and IP
TCPConnection* tcp_find_connection(uint16_t local_port, uint16_t remote_port, uint8_t* remote_ip) {
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].active &&
            tcp_connections[i].local_port == local_port &&
            tcp_connections[i].remote_port == remote_port &&
            memcmp(tcp_connections[i].remote_ip, remote_ip, IP_ADDR_LEN) == 0) {
            return &tcp_connections[i];
        }
    }
    return NULL;
}

// Find listening connection
TCPConnection* tcp_find_listener(uint16_t local_port) {
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (tcp_connections[i].active &&
            tcp_connections[i].listening &&
            tcp_connections[i].local_port == local_port) {
            return &tcp_connections[i];
        }
    }
    return NULL;
}

// Send IP packet (wraps in Ethernet frame)
void send_ip_packet(uint8_t* dest_ip, uint8_t* packet, uint16_t len) {
    // Use routing logic to get next hop MAC (gateway for remote, direct for local)
    uint8_t* dest_mac = route_get_next_hop_mac(dest_ip);
    if (!dest_mac) {
        // MAC not in cache - would need to do ARP request
        // For now, just drop the packet
        return;
    }
    
    // Build Ethernet frame
    uint8_t frame[sizeof(EthernetHeader) + len];
    EthernetHeader* eth = (EthernetHeader*)frame;
    
    // Copy source and dest MAC
    for (int i = 0; i < ETH_ALEN; i++) {
        eth->dest[i] = dest_mac[i];
        eth->src[i] = net_device.mac[i];
    }
    eth->type = htons(ETH_P_IP);
    
    // Copy IP packet
    for (uint16_t i = 0; i < len; i++) {
        frame[sizeof(EthernetHeader) + i] = packet[i];
    }
    
    net_send(frame, sizeof(EthernetHeader) + len);
}

// Send TCP packet
void tcp_send_packet(TCPConnection* conn, uint8_t flags, uint8_t* data, uint16_t data_len) {
    uint8_t packet[1500];
    uint16_t total_len = sizeof(IPHeader) + sizeof(TCPHeader) + data_len;
    
    if (total_len > sizeof(packet)) return;
    
    // IP header
    IPHeader* ip = (IPHeader*)packet;
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = htons(total_len);
    ip->id = htons(ip_id_counter++);
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = 6;  // TCP
    ip->checksum = 0;
    memcpy(ip->src_ip, net_device.ip, IP_ADDR_LEN);
    memcpy(ip->dest_ip, conn->remote_ip, IP_ADDR_LEN);
    
    ip->checksum = ip_checksum(ip, sizeof(IPHeader));
    
    // TCP header
    TCPHeader* tcp = (TCPHeader*)(packet + sizeof(IPHeader));
    tcp->src_port = htons(conn->local_port);
    tcp->dest_port = htons(conn->remote_port);
    tcp->seq_num = htonl(conn->send_seq);
    tcp->ack_num = htonl(conn->send_ack);
    tcp->data_offset = 0x50;  // 20 bytes, no options
    tcp->flags = flags;
    tcp->window = htons(TCP_RECV_BUFFER_SIZE - conn->recv_len);
    tcp->checksum = 0;
    tcp->urgent_ptr = 0;
    
    // Copy data
    if (data && data_len > 0) {
        memcpy(packet + sizeof(IPHeader) + sizeof(TCPHeader), data, data_len);
    }
    
    // Calculate TCP checksum
    tcp->checksum = htons(tcp_checksum(net_device.ip, conn->remote_ip, tcp, data, sizeof(TCPHeader) + data_len));
    
    // Send packet
    send_ip_packet(conn->remote_ip, packet, total_len);
    
    // Update sequence number if we sent data or SYN/FIN
    if (data_len > 0 || (flags & (TCP_SYN | TCP_FIN))) {
        conn->send_seq += data_len;
        if (flags & TCP_SYN) conn->send_seq++;
        if (flags & TCP_FIN) conn->send_seq++;
    }
}

// Handle incoming TCP packet
void handle_tcp(uint8_t* src_ip, uint8_t* data, uint16_t len) {
    if (len < sizeof(TCPHeader)) return;
    
    TCPHeader* tcp = (TCPHeader*)data;
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    uint32_t seq = ntohl(tcp->seq_num);
    uint32_t ack = ntohl(tcp->ack_num);
    uint8_t flags = tcp->flags;
    
    uint8_t header_len = (tcp->data_offset >> 4) * 4;
    uint8_t* payload = data + header_len;
    uint16_t payload_len = len - header_len;
    
    // Find connection
    TCPConnection* conn = tcp_find_connection(dest_port, src_port, src_ip);
    
    // If no connection, check for listener
    if (!conn && (flags & TCP_SYN) && !(flags & TCP_ACK)) {
        TCPConnection* listener = tcp_find_listener(dest_port);
        if (listener) {
            // Accept new connection
            conn = tcp_find_free_slot();
            if (conn) {
                conn->local_port = dest_port;
                conn->remote_port = src_port;
                memcpy(conn->remote_ip, src_ip, IP_ADDR_LEN);
                conn->state = TCP_SYN_RECEIVED;
                conn->recv_seq = seq + 1;
                conn->send_ack = seq + 1;
                conn->send_seq = tcp_isn++;
                
                // Send SYN-ACK
                tcp_send_packet(conn, TCP_SYN | TCP_ACK, NULL, 0);
                return;
            }
        }
        // No listener or no free slots - send RST
        return;
    }
    
    if (!conn) {
        // Unknown connection - ignore or send RST
        return;
    }
    
    // Handle based on state
    switch (conn->state) {
        case TCP_SYN_SENT:
            if ((flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
                // Received SYN-ACK
                conn->recv_seq = seq + 1;
                conn->send_ack = seq + 1;
                conn->state = TCP_ESTABLISHED;
                
                // Send ACK
                tcp_send_packet(conn, TCP_ACK, NULL, 0);
            }
            break;
            
        case TCP_SYN_RECEIVED:
            if (flags & TCP_ACK) {
                conn->state = TCP_ESTABLISHED;
            }
            break;
            
        case TCP_ESTABLISHED:
            // Handle data
            if (payload_len > 0) {
                uint16_t space = TCP_RECV_BUFFER_SIZE - conn->recv_len;
                uint16_t copy_len = (payload_len < space) ? payload_len : space;
                
                if (copy_len > 0) {
                    memcpy(conn->recv_buffer + conn->recv_len, payload, copy_len);
                    conn->recv_len += copy_len;
                    conn->recv_seq += copy_len;
                    conn->send_ack = conn->recv_seq;
                }
                
                // Send ACK
                tcp_send_packet(conn, TCP_ACK, NULL, 0);
            }
            
            // Handle FIN
            if (flags & TCP_FIN) {
                conn->recv_seq++;
                conn->send_ack = conn->recv_seq;
                conn->fin_received = true;
                conn->state = TCP_CLOSE_WAIT;
                
                // Send ACK
                tcp_send_packet(conn, TCP_ACK, NULL, 0);
            }
            
            // Handle ACK for our data
            if (flags & TCP_ACK) {
                // Data acknowledged - could clear send buffer here
            }
            break;
            
        case TCP_FIN_WAIT_1:
            if (flags & TCP_ACK) {
                conn->state = TCP_FIN_WAIT_2;
            }
            if (flags & TCP_FIN) {
                conn->recv_seq++;
                conn->send_ack = conn->recv_seq;
                tcp_send_packet(conn, TCP_ACK, NULL, 0);
                conn->state = TCP_TIME_WAIT;
                // Should wait, then close
                conn->active = false;
            }
            break;
            
        case TCP_FIN_WAIT_2:
            if (flags & TCP_FIN) {
                conn->recv_seq++;
                conn->send_ack = conn->recv_seq;
                tcp_send_packet(conn, TCP_ACK, NULL, 0);
                conn->state = TCP_TIME_WAIT;
                conn->active = false;
            }
            break;
            
        case TCP_CLOSE_WAIT:
            // Waiting for application to close
            break;
            
        case TCP_LAST_ACK:
            if (flags & TCP_ACK) {
                conn->active = false;
            }
            break;
            
        default:
            break;
    }
}

// ====================
// TCP API FUNCTIONS
// ====================

// Connect to remote TCP server
int tcp_connect(uint8_t* remote_ip, uint16_t remote_port, uint16_t local_port) {
    // Determine next hop (gateway for remote, direct for local network)
    uint8_t* arp_target = remote_ip;
    
    // Check if destination is on local network
    bool on_local_network = true;
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        if ((remote_ip[i] & net_device.netmask[i]) != (net_device.ip[i] & net_device.netmask[i])) {
            on_local_network = false;
            break;
        }
    }
    
    if (!on_local_network) {
        if (net_device.gateway[0] == 0) {
            return -1;  // No gateway configured
        }
        arp_target = net_device.gateway;
    }
    
    // Check if we have the MAC address in ARP cache
    uint8_t* next_hop_mac = arp_lookup(arp_target);
    if (!next_hop_mac) {
        // Need to do ARP resolution first
        arp_send_request(arp_target);
        
        // Wait for ARP reply (up to 3 seconds)
        int arp_attempts = 0;
        while (!next_hop_mac && arp_attempts < 30) {
            net_poll();
            next_hop_mac = arp_lookup(arp_target);
            if (next_hop_mac) break;
            
            delay_ms(100);  // 100ms delay per attempt
            arp_attempts++;
        }
        
        if (!next_hop_mac) {
            return -1;  // ARP failed
        }
    }
    
    // Find free connection slot
    TCPConnection* conn = tcp_find_free_slot();
    if (!conn) {
        return -1;  // No free slots
    }
    
    // Initialize connection
    memcpy(conn->remote_ip, remote_ip, IP_ADDR_LEN);
    conn->remote_port = remote_port;
    conn->local_port = local_port ? local_port : allocate_ephemeral_port();
    conn->state = TCP_SYN_SENT;
    conn->send_seq = tcp_isn++;
    conn->send_ack = 0;
    conn->recv_seq = 0;
    conn->recv_len = 0;
    conn->send_len = 0;
    conn->listening = false;
    conn->fin_received = false;
    
    // Send SYN
    tcp_send_packet(conn, TCP_SYN, NULL, 0);
    
    // Return connection ID
    return conn - tcp_connections;
}

// Listen on local port
int tcp_listen(uint16_t local_port) {
    // Check if already listening
    if (tcp_find_listener(local_port)) {
        return -1;  // Already listening
    }
    
    // Find free connection slot
    TCPConnection* conn = tcp_find_free_slot();
    if (!conn) {
        return -1;  // No free slots
    }
    
    // Initialize listener
    conn->local_port = local_port;
    conn->state = TCP_LISTEN;
    conn->listening = true;
    
    // Return connection ID
    return conn - tcp_connections;
}

// Send data on established connection
int tcp_send_data(int conn_id, uint8_t* data, uint16_t len) {
    if (conn_id < 0 || conn_id >= MAX_TCP_CONNECTIONS) {
        return -1;
    }
    
    TCPConnection* conn = &tcp_connections[conn_id];
    
    if (!conn->active || conn->state != TCP_ESTABLISHED) {
        return -1;
    }
    
    // Send packet with data
    tcp_send_packet(conn, TCP_ACK | TCP_PSH, data, len);
    
    return len;
}

// Receive data from connection
int tcp_receive_data(int conn_id, uint8_t* buffer, uint16_t max_len) {
    if (conn_id < 0 || conn_id >= MAX_TCP_CONNECTIONS) {
        return -1;
    }
    
    TCPConnection* conn = &tcp_connections[conn_id];
    
    if (!conn->active) {
        return -1;
    }
    
    uint16_t copy_len = (conn->recv_len < max_len) ? conn->recv_len : max_len;
    
    if (copy_len > 0) {
        memcpy(buffer, conn->recv_buffer, copy_len);
        
        // Shift remaining data
        if (copy_len < conn->recv_len) {
            memmove(conn->recv_buffer, conn->recv_buffer + copy_len, conn->recv_len - copy_len);
        }
        
        conn->recv_len -= copy_len;
    }
    
    return copy_len;
}

// Close TCP connection
void tcp_close(int conn_id) {
    if (conn_id < 0 || conn_id >= MAX_TCP_CONNECTIONS) {
        return;
    }
    
    TCPConnection* conn = &tcp_connections[conn_id];
    
    if (!conn->active) {
        return;
    }
    
    if (conn->state == TCP_ESTABLISHED) {
        // Send FIN
        tcp_send_packet(conn, TCP_FIN | TCP_ACK, NULL, 0);
        conn->state = TCP_FIN_WAIT_1;
    } else if (conn->state == TCP_CLOSE_WAIT) {
        // Send FIN
        tcp_send_packet(conn, TCP_FIN | TCP_ACK, NULL, 0);
        conn->state = TCP_LAST_ACK;
    } else {
        // Just close
        if (conn->local_port >= 49152) {
            free_ephemeral_port(conn->local_port);
        }
        conn->active = false;
    }
}

// Check if connection is established
bool tcp_is_established(int conn_id) {
    if (conn_id < 0 || conn_id >= MAX_TCP_CONNECTIONS) {
        return false;
    }
    
    return tcp_connections[conn_id].active && 
           tcp_connections[conn_id].state == TCP_ESTABLISHED;
}

// Get connection state
TCPState tcp_get_state(int conn_id) {
    if (conn_id < 0 || conn_id >= MAX_TCP_CONNECTIONS) {
        return TCP_CLOSED;
    }
    
    return tcp_connections[conn_id].state;
}

// ====================
// EPHEMERAL PORT MANAGEMENT
// ====================

// Allocate an ephemeral port
uint16_t allocate_ephemeral_port(void) {
    // Try to find a free port
    for (int attempts = 0; attempts < (EPHEMERAL_PORT_END - EPHEMERAL_PORT_START); attempts++) {
        uint16_t port = next_ephemeral_port++;
        if (next_ephemeral_port >= EPHEMERAL_PORT_END) {
            next_ephemeral_port = EPHEMERAL_PORT_START;
        }
        
        // Check if port is already in use
        bool in_use = false;
        for (int i = 0; i < MAX_PORT_BINDINGS; i++) {
            if (port_bindings[i].in_use && port_bindings[i].port == port) {
                in_use = true;
                break;
            }
        }
        
        if (!in_use) {
            // Find empty slot in binding table
            for (int i = 0; i < MAX_PORT_BINDINGS; i++) {
                if (!port_bindings[i].in_use) {
                    port_bindings[i].port = port;
                    port_bindings[i].in_use = true;
                    port_bindings[i].handler = 0;
                    return port;
                }
            }
            // No slots available, but port is free - return it anyway
            return port;
        }
    }
    
    // All ports exhausted (shouldn't happen)
    return 0;
}

// Free an ephemeral port
void free_ephemeral_port(uint16_t port) {
    for (int i = 0; i < MAX_PORT_BINDINGS; i++) {
        if (port_bindings[i].in_use && port_bindings[i].port == port) {
            port_bindings[i].in_use = false;
            port_bindings[i].port = 0;
            port_bindings[i].handler = 0;
            return;
        }
    }
}

// Check if a port is in use
bool is_port_in_use(uint16_t port) {
    for (int i = 0; i < MAX_PORT_BINDINGS; i++) {
        if (port_bindings[i].in_use && port_bindings[i].port == port) {
            return true;
        }
    }
    return false;
}

// ====================
// DNS CLIENT
// ====================

// Initialize DNS cache
void dns_cache_init(void) {
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        dns_cache[i].valid = false;
        dns_cache[i].name[0] = '\0';
    }
}

// Look up a name in the cache
bool dns_cache_lookup(const char* name, uint8_t* ip) {
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_cache[i].valid && strcmp(dns_cache[i].name, name) == 0) {
            for (int j = 0; j < IP_ADDR_LEN; j++) {
                ip[j] = dns_cache[i].ip[j];
            }
            return true;
        }
    }
    return false;
}

// Add entry to DNS cache
void dns_cache_add(const char* name, uint8_t* ip) {
    terminal_writestring("DNS: Adding to cache: '");
    terminal_writestring(name);
    terminal_writestring("' -> ");
    for (int j = 0; j < IP_ADDR_LEN; j++) {
        char buf[4];
        itoa_helper(ip[j], buf);
        terminal_writestring(buf);
        if (j < 3) terminal_putchar('.');
    }
    terminal_putchar('\n');
    
    // Find empty slot or oldest entry
    int slot = -1;
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (!dns_cache[i].valid) {
            slot = i;
            break;
        }
    }
    
    // If no empty slot, use slot 0 (simple FIFO)
    if (slot == -1) {
        slot = 0;
    }
    
    // Add to cache
    strncpy(dns_cache[slot].name, name, DNS_MAX_NAME - 1);
    dns_cache[slot].name[DNS_MAX_NAME - 1] = '\0';
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        dns_cache[slot].ip[i] = ip[i];
    }
    dns_cache[slot].valid = true;
    dns_cache[slot].timestamp = 0;  // Not using TTL yet
}

// Encode DNS name (e.g., "google.com" -> "\x06google\x03com\x00")
int dns_encode_name(const char* name, uint8_t* buffer) {
    int pos = 0;
    int label_start = 0;
    int i = 0;
    
    while (name[i]) {
        if (name[i] == '.') {
            // Write label length
            int len = i - label_start;
            buffer[pos++] = len;
            // Write label
            for (int j = 0; j < len; j++) {
                buffer[pos++] = name[label_start + j];
            }
            label_start = i + 1;
        }
        i++;
    }
    
    // Write final label
    int len = i - label_start;
    buffer[pos++] = len;
    for (int j = 0; j < len; j++) {
        buffer[pos++] = name[label_start + j];
    }
    
    // Null terminator
    buffer[pos++] = 0;
    
    return pos;
}

// Send DNS query
void dns_send_query(const char* name) {
    if (!net_device.initialized || net_device.dns[0] == 0) {
        terminal_writestring("DNS: No DNS server configured\n");
        return;
    }
    
    // Store query name for response handler
    strncpy(dns_current_query, name, DNS_MAX_NAME - 1);
    dns_current_query[DNS_MAX_NAME - 1] = '\0';
    
    // Show DNS server being used
    terminal_writestring("DNS: Using nameserver ");
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        char buf[4];
        itoa_helper(net_device.dns[i], buf);
        terminal_writestring(buf);
        if (i < 3) terminal_putchar('.');
    }
    terminal_putchar('\n');
    
    // Determine next hop (local DNS or via gateway)
    uint8_t* arp_target;
    if (is_local_subnet(net_device.dns)) {
        terminal_writestring("DNS: Server is on local network\n");
        arp_target = net_device.dns;
    } else {
        terminal_writestring("DNS: Server is remote, routing via gateway\n");
        if (net_device.gateway[0] == 0) {
            terminal_writestring("DNS: No gateway configured\n");
            return;
        }
        arp_target = net_device.gateway;
    }
    
    // Check if next hop MAC is in ARP cache
    uint8_t* target_mac = arp_lookup(arp_target);
    if (!target_mac) {
        terminal_writestring("DNS: Resolving next hop MAC via ARP...\n");
        arp_send_request(arp_target);
        
        // Wait for ARP reply (1 second timeout)
        int arp_attempts = 0;
        while (!target_mac && arp_attempts < 10) {
            net_poll();
            target_mac = arp_lookup(arp_target);
            if (target_mac) break;
            delay_ms(100);
            arp_attempts++;
        }
        
        if (!target_mac) {
            terminal_writestring("DNS: Cannot reach DNS server (ARP timeout)\n");
            return;
        }
        terminal_writestring("DNS: ARP resolved\n");
    } else {
        terminal_writestring("DNS: Using cached MAC address\n");
    }
    
    uint8_t packet[512];
    int pos = 0;
    
    // DNS header
    DNSHeader* header = (DNSHeader*)packet;
    header->id = htons(dns_query_id++);
    header->flags = htons(0x0100);  // Standard query, recursion desired
    header->qdcount = htons(1);     // 1 question
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;
    pos += sizeof(DNSHeader);
    
    // Question section
    pos += dns_encode_name(name, packet + pos);
    
    // QTYPE (A record)
    packet[pos++] = 0;
    packet[pos++] = DNS_TYPE_A;
    
    // QCLASS (IN)
    packet[pos++] = 0;
    packet[pos++] = DNS_CLASS_IN;
    
    // Allocate ephemeral port for this query
    dns_query_port = allocate_ephemeral_port();
    if (dns_query_port == 0) {
        terminal_writestring("DNS: Failed to allocate port\n");
        return;
    }
    
    // Send via UDP to DNS server port 53 from allocated ephemeral port
    terminal_writestring("DNS: Sending query packet (");
    char buf[8];
    itoa_helper(pos, buf);
    terminal_writestring(buf);
    terminal_writestring(" bytes) from port ");
    itoa_helper(dns_query_port, buf);
    terminal_writestring(buf);
    terminal_writestring("...\n");
    send_udp(net_device.dns, target_mac, dns_query_port, 53, packet, pos);
    terminal_writestring("DNS: Query sent, waiting for response...\n");
}

// Parse DNS response
void handle_dns_response(uint8_t* data, uint16_t len) {
    terminal_writestring("DNS: Parsing response (");
    char buf[8];
    itoa_helper(len, buf);
    terminal_writestring(buf);
    terminal_writestring(" bytes)\n");
    
    if (len < sizeof(DNSHeader)) {
        terminal_writestring("DNS: Response too short\n");
        return;
    }
    
    DNSHeader* header = (DNSHeader*)data;
    uint16_t id = ntohs(header->id);
    uint16_t flags = ntohs(header->flags);
    uint16_t qdcount = ntohs(header->qdcount);
    uint16_t ancount = ntohs(header->ancount);
    
    terminal_writestring("DNS: ID=");
    itoa_helper(id, buf);
    terminal_writestring(buf);
    terminal_writestring(" Flags=0x");
    itoa_hex(flags, buf);
    terminal_writestring(buf);
    terminal_writestring(" Questions=");
    itoa_helper(qdcount, buf);
    terminal_writestring(buf);
    terminal_writestring(" Answers=");
    itoa_helper(ancount, buf);
    terminal_writestring(buf);
    terminal_putchar('\n');
    
    if (ancount == 0) {
        terminal_writestring("DNS: No answers in response\n");
        return;
    }
    
    int pos = sizeof(DNSHeader);
    terminal_writestring("DNS: Skipping question section...\n");
    
    // Skip question section
    while (pos < len && data[pos] != 0) {
        if (data[pos] >= 0xC0) {
            // Compressed name pointer (2 bytes)
            terminal_writestring("DNS: Found compressed pointer in question\n");
            pos += 2;
            break;
        } else {
            // Regular label
            pos += data[pos] + 1;
        }
    }
    if (pos < len && data[pos] == 0) pos++;  // Skip null terminator
    
    pos += 4;  // Skip QTYPE and QCLASS
    
    terminal_writestring("DNS: Answer section starts at offset ");
    itoa_helper(pos, buf);
    terminal_writestring(buf);
    terminal_putchar('\n');
    
    // Parse answer section
    for (int i = 0; i < ancount && pos + 12 <= len; i++) {
        terminal_writestring("DNS: Parsing answer ");
        itoa_helper(i + 1, buf);
        terminal_writestring(buf);
        terminal_writestring(" at offset ");
        itoa_helper(pos, buf);
        terminal_writestring(buf);
        terminal_putchar('\n');
        
        // Skip answer name (use same logic as question)
        bool compressed = false;
        while (pos < len && data[pos] != 0) {
            if (data[pos] >= 0xC0) {
                terminal_writestring("DNS: Compressed pointer in answer\n");
                pos += 2;
                compressed = true;
                break;
            } else {
                pos += data[pos] + 1;
            }
        }
        // Only skip null terminator if we didn't have a compressed pointer
        if (!compressed && pos < len && data[pos] == 0) pos++;
        
        // Read TYPE, CLASS, TTL, RDLENGTH
        if (pos + 10 > len) {
            terminal_writestring("DNS: Not enough data for answer header\n");
            break;
        }
        
        uint16_t rtype = (data[pos] << 8) | data[pos + 1];
        uint16_t rclass = (data[pos + 2] << 8) | data[pos + 3];
        pos += 8;  // Skip TYPE, CLASS, TTL
        uint16_t rdlength = (data[pos] << 8) | data[pos + 1];
        pos += 2;
        
        terminal_writestring("DNS: TYPE=");
        itoa_helper(rtype, buf);
        terminal_writestring(buf);
        terminal_writestring(" CLASS=");
        itoa_helper(rclass, buf);
        terminal_writestring(buf);
        terminal_writestring(" RDLEN=");
        itoa_helper(rdlength, buf);
        terminal_writestring(buf);
        terminal_putchar('\n');
        
        // If this is an A record with 4 bytes of data
        if (rtype == DNS_TYPE_A && rdlength == 4 && pos + 4 <= len) {
            uint8_t ip[IP_ADDR_LEN];
            for (int j = 0; j < 4; j++) {
                ip[j] = data[pos + j];
            }
            
            terminal_writestring("DNS: Found A record: ");
            for (int j = 0; j < 4; j++) {
                itoa_helper(ip[j], buf);
                terminal_writestring(buf);
                if (j < 3) terminal_putchar('.');
            }
            terminal_putchar('\n');
            
            // Add to cache
            dns_cache_add(dns_current_query, ip);
            
            return;  // Got our answer
        }
        
        pos += rdlength;
    }
    
    terminal_writestring("DNS: Finished parsing, no A record found\n");
}

// Resolve a domain name (with caching)
bool dns_resolve(const char* name, uint8_t* ip) {
    // Check cache first
    if (dns_cache_lookup(name, ip)) {
        terminal_writestring("DNS: Found in cache\n");
        return true;
    }
    
    // Send DNS query (allocates a port)
    terminal_writestring("DNS: Querying ");
    terminal_writestring(name);
    terminal_writestring("...\n");
    dns_send_query(name);
    
    // Wait for response (5 seconds)
    int attempts = 0;
    bool success = false;
    while (attempts < 500) {
        if (interrupt_command) {
            terminal_writestring("DNS: Interrupted\n");
            // Free the allocated port
            if (dns_query_port != 0) {
                free_ephemeral_port(dns_query_port);
                dns_query_port = 0;
            }
            return false;
        }
        net_poll();
        delay_ms(10);
        attempts++;
        
        // Check cache again (will be populated by response handler)
        if (dns_cache_lookup(name, ip)) {
            success = true;
            break;
        }
    }
    
    // Free the allocated port
    if (dns_query_port != 0) {
        free_ephemeral_port(dns_query_port);
        dns_query_port = 0;
    }
    
    if (!success) {
        terminal_writestring("DNS: Query timeout\n");
    }
    return success;
}

// ====================
// FILESYSTEM
// ====================

int alloc_entry(const char* name, int type, int parent) {
    if (entry_count >= MAX_ENTRIES) return -1;
    int idx = entry_count++;
    
    strncpy(entries[idx].name, name, MAX_NAME - 1);
    entries[idx].name[MAX_NAME - 1] = '\0';
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
    if (child_idx < 0 || child_idx >= entry_count) return -1;
    
    // Prevent adding a directory as its own child
    if (dir_idx == child_idx) return -1;
    
    // Check if this child is already in the list
    int check = entries[dir_idx].first_child;
    while (check != -1) {
        if (check == child_idx) return -1;
        check = entries[check].next_sibling;
    }
    
    entries[child_idx].next_sibling = entries[dir_idx].first_child;
    entries[dir_idx].first_child = child_idx;
    
    return 0;
}

void init_fs(void) {
    // Initialize all entries
    for (int i = 0; i < MAX_ENTRIES; i++) {
        entries[i].first_child = -1;
        entries[i].next_sibling = -1;
        entries[i].parent = -1;
        entries[i].name[0] = '\0';
    }
    
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
    terminal_writestring("=== slopOS - slide on in ===\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("Default vibers:\n");
    terminal_writestring("  admin / slopOS123 (the boss)\n");
    terminal_writestring("  user  / password  (just chillin)\n\n");
    
    while (attempts < max_attempts) {
        terminal_writestring("who are you? ");
        read_line(username, sizeof(username));
        
        terminal_writestring("prove it: ");
        read_password(password, sizeof(password));
        
        int user_idx = find_user(username);
        if (user_idx != -1 && verify_password(user_idx, password)) {
            auth.current_user = user_idx;
            auth.logged_in = true;
            
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            terminal_writestring("ayy you made it! welcome back, ");
            terminal_writestring(auth.users[user_idx].username);
            terminal_writestring("!\n");
            
            if (auth.users[user_idx].is_admin) {
                terminal_writestring("you got the keys to the castle!\n");
            }
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
            terminal_writestring("\n");
            
            memset(password, 0, sizeof(password));
            return true;
        }
        
        attempts++;
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("nah that ain't it chief.\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        
        memset(password, 0, sizeof(password));
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    terminal_writestring("too many bad vibes. system's done with you.\n");
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

// --------------------
// Help Command
// --------------------
// Moved to kernel/commands/help_command.c

// --------------------
// Filesystem Commands
// --------------------
// Moved to kernel/commands/fs_commands.c

// --------------------
// User Commands  
// --------------------
// Moved to kernel/commands/user_commands.c

// Helper to convert number to string
void itoa_helper(int n, char* buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    int i = 0;
    int is_neg = 0;
    if (n < 0) {
        is_neg = 1;
        n = -n;
    }
    
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    if (is_neg) buf[i++] = '-';
    buf[i] = '\0';
    
    // Reverse
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = tmp;
    }
}

void itoa_hex(int n, char* buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    int i = 0;
    unsigned int un = (unsigned int)n;
    
    while (un > 0) {
        int digit = un & 0xF;
        buf[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        un >>= 4;
    }
    
    buf[i] = '\0';
    
    // Reverse
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = tmp;
    }
}

// Network Commands - Moved to kernel/commands/network_commands.c

// Parse IP address from string
bool parse_ip(const char* str, uint8_t* ip) {
    int octet = 0;
    int value = 0;
    
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            value = value * 10 + (*str - '0');
        } else if (*str == '.') {
            if (octet >= 4 || value > 255) return false;
            ip[octet++] = value;
            value = 0;
        } else {
            return false;
        }
        str++;
    }
    
    if (octet != 3 || value > 255) return false;
    ip[octet] = value;
    return true;
}

void cmd_flop(char* arg1, char* arg2) {
    if (!net_device.initialized) {
        terminal_writestring("yo network ain't ready yet\n");
        return;
    }
    
    // Check for listen mode: flop -l <port>
    if (arg1 && strcmp(arg1, "-l") == 0) {
        if (!arg2) {
            terminal_writestring("Usage: flop -l <port>\n");
            return;
        }
        
        int port = atoi(arg2);
        if (port <= 0 || port > 65535) {
            terminal_writestring("nah that port number's bogus\n");
            return;
        }
        
        int conn_id = tcp_listen(port);
        if (conn_id < 0) {
            terminal_writestring("couldn't start flopping on that port\n");
            return;
        }
        
        flop_conn_id = conn_id;
        terminal_writestring("aight we flopping on port ");
        char buf[16];
        itoa_helper(port, buf);
        terminal_writestring(buf);
        terminal_writestring(" ... waiting for connections\n");
        terminal_writestring("(hit CTRL+C to bail)\n");
        return;
    }
    
    // Connect mode: flop <ip> <port>
    if (!arg1 || !arg2) {
        terminal_writestring("Usage:\n");
        terminal_writestring("  flop <ip> <port>     - connect to remote host\n");
        terminal_writestring("  flop -l <port>       - listen on local port\n");
        return;
    }
    
    // Parse IP address
    uint8_t remote_ip[IP_ADDR_LEN];
    int parts[4];
    int count = 0;
    char* tok = arg1;
    
    for (int i = 0; i < 4; i++) {
        char* dot = strchr(tok, '.');
        if (i < 3 && !dot) {
            terminal_writestring("invalid IP address format\n");
            return;
        }
        
        if (dot) *dot = '\0';
        parts[count++] = atoi(tok);
        if (dot) {
            *dot = '.';
            tok = dot + 1;
        }
    }
    
    if (count != 4) {
        terminal_writestring("invalid IP address\n");
        return;
    }
    
    for (int i = 0; i < 4; i++) {
        remote_ip[i] = parts[i];
    }
    
    int port = atoi(arg2);
    if (port <= 0 || port > 65535) {
        terminal_writestring("nah that port number's bogus\n");
        return;
    }
    
    terminal_writestring("tryna flop with ");
    terminal_writestring(arg1);
    terminal_writestring(":");
    char buf[16];
    itoa_helper(port, buf);
    terminal_writestring(buf);
    terminal_writestring("...\n");
    
    terminal_writestring("resolving MAC address...\n");
    
    int conn_id = tcp_connect(remote_ip, port, 0);
    if (conn_id < 0) {
        terminal_writestring("couldn't make the connection happen (ARP failed or no slots)\n");
        terminal_writestring("make sure you ran 'gimmeip' and can 'poke' the gateway first\n");
        return;
    }
    
    terminal_writestring("MAC resolved! ");
    flop_conn_id = conn_id;
    
    // Wait a bit for connection to establish
    terminal_writestring("sending SYN, waiting for handshake...\n");
    
    // Poll for connection - need to call net_poll() to process packets!
    for (int i = 0; i < 50; i++) {  // 5 seconds max (50 * 100ms)
        // Process incoming packets
        net_poll();
        
        if (tcp_is_established(conn_id)) {
            terminal_writestring("ayy we connected! start typing (CTRL+C to bail)\n");
            
            // Enter interactive mode - stay in a loop sending data
            char line_buffer[MAX_CMD_LEN];
            while (tcp_is_established(conn_id)) {
                size_t pos = 0;
                
                // Read a line of input
                while (1) {
                    // Check for CTRL+C
                    if (interrupt_command) {
                        interrupt_command = false;
                        terminal_writestring("\nclosing connection...\n");
                        tcp_close(conn_id);
                        flop_conn_id = -1;
                        return;
                    }
                    
                    // Poll for incoming data while waiting for input
                    net_poll();
                    uint8_t recv_buffer[256];
                    int recv_len = tcp_receive_data(conn_id, recv_buffer, sizeof(recv_buffer));
                    if (recv_len > 0) {
                        // Display received data (character by character for safety)
                        for (int i = 0; i < recv_len; i++) {
                            char c = recv_buffer[i];
                            // Print printable characters, newlines, and carriage returns
                            if (c >= 32 && c < 127) {
                                terminal_putchar(c);
                            } else if (c == '\n') {
                                terminal_putchar('\n');
                            } else if (c == '\r') {
                                terminal_putchar('\r');
                            } else if (c == '\t') {
                                terminal_putchar('\t');
                            }
                            // Skip other control characters
                        }
                    }
                    
                    // Check for keyboard input
                    char c = keyboard_getchar();
                    if (c == 0) {
                        // Small delay to avoid busy-waiting
                        delay_ms(10);
                        continue;
                    }
                    
                    if (c == '\n') {
                        terminal_putchar('\n');
                        line_buffer[pos] = '\0';
                        
                        // Send the line
                        if (pos > 0) {
                            line_buffer[pos] = '\n';
                            tcp_send_data(conn_id, (uint8_t*)line_buffer, pos + 1);
                        }
                        
                        // After sending, check for any immediate response
                        for (int i = 0; i < 10; i++) {
                            net_poll();
                            uint8_t resp_buffer[256];
                            int resp_len = tcp_receive_data(conn_id, resp_buffer, sizeof(resp_buffer));
                            if (resp_len > 0) {
                                // Display received data
                                for (int j = 0; j < resp_len; j++) {
                                    char c = resp_buffer[j];
                                    if (c >= 32 && c < 127) {
                                        terminal_putchar(c);
                                    } else if (c == '\n') {
                                        terminal_putchar('\n');
                                    } else if (c == '\r') {
                                        terminal_putchar('\r');
                                    } else if (c == '\t') {
                                        terminal_putchar('\t');
                                    }
                                }
                            }
                            delay_ms(10);
                        }
                        break;
                    }
                    
                    if (c == '\b') {
                        if (pos > 0) {
                            terminal_putchar('\b');
                            pos--;
                        }
                    } else if (c >= 32 && c < 127 && pos < MAX_CMD_LEN - 1) {
                        terminal_putchar(c);
                        line_buffer[pos++] = c;
                    }
                }
            }
            
            terminal_writestring("connection closed\n");
            flop_conn_id = -1;
            return;
        }
        
        // 100ms delay between polls
        delay_ms(100);
    }
    
    TCPState state = tcp_get_state(conn_id);
    if (state != TCP_ESTABLISHED) {
        terminal_writestring("connection timed out or rejected\n");
        tcp_close(conn_id);
        flop_conn_id = -1;
    }
}

// GUI Commands - Moved to kernel/commands/gui_commands.c

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
    
    // Reset interrupt flag before executing command
    interrupt_command = false;
    
    if (strcmp(cmd, "whatdo") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "peek") == 0) {
        cmd_ls();
    } else if (strcmp(cmd, "cook") == 0) {
        char* arg = strtok(NULL, " ");
        if (arg) cmd_mkdir(arg);
        else terminal_writestring("cook: missing name\n");
    } else if (strcmp(cmd, "yeet") == 0) {
        char* arg = strtok(NULL, " ");
        if (arg) cmd_touch(arg);
        else terminal_writestring("yeet: missing name\n");
    } else if (strcmp(cmd, "bounce") == 0) {
        char* arg = strtok(NULL, " ");
        if (arg) cmd_cd(arg);
        else terminal_writestring("bounce: missing name\n");
    } else if (strcmp(cmd, "whereami") == 0) {
        cmd_pwd();
    } else if (strcmp(cmd, "me") == 0) {
        cmd_whoami();
    } else if (strcmp(cmd, "whosthere") == 0) {
        cmd_listusers();
    } else if (strcmp(cmd, "mynet") == 0) {
        cmd_ifconfig();
    } else if (strcmp(cmd, "arp") == 0) {
        cmd_arp();
    } else if (strcmp(cmd, "netstat") == 0) {
        cmd_netstat();
    } else if (strcmp(cmd, "route") == 0) {
        cmd_route();
    } else if (strcmp(cmd, "poke") == 0) {
        char* ip_arg = strtok(NULL, " ");
        if (!ip_arg) {
            terminal_writestring("Usage: poke <ip> [count]\n");
            terminal_writestring("  count: number of pokes to send (default: 5)\n");
        } else {
            char* count_arg = strtok(NULL, " ");
            int count = 5; // default
            if (count_arg) {
                count = 0;
                // Simple atoi
                for (int i = 0; count_arg[i] >= '0' && count_arg[i] <= '9'; i++) {
                    count = count * 10 + (count_arg[i] - '0');
                }
                if (count < 1) count = 5;
                if (count > 20) count = 20; // Max 20 pokes
            }
            cmd_ping(ip_arg, count);
        }
    } else if (strcmp(cmd, "gimmeip") == 0) {
        cmd_dhcp();
    } else if (strcmp(cmd, "wherez") == 0) {
        char* hostname = strtok(NULL, " ");
        if (!hostname) {
            terminal_writestring("Usage: wherez <hostname>\n");
        } else {
            cmd_nslookup(hostname);
        }
    } else if (strcmp(cmd, "flop") == 0) {
        char* arg1 = strtok(NULL, " ");
        char* arg2 = strtok(NULL, " ");
        cmd_flop(arg1, arg2);
    } else if (strcmp(cmd, "passwd") == 0) {
        cmd_passwd();
    } else if (strcmp(cmd, "recruit") == 0) {
        char* user = strtok(NULL, " ");
        char* pass = strtok(NULL, " ");
        if (user && pass) cmd_adduser(user, pass);
        else terminal_writestring("recruit: usage: recruit <username> <password>\n");
    } else if (strcmp(cmd, "kickout") == 0) {
        char* user = strtok(NULL, " ");
        if (user) cmd_deluser(user);
        else terminal_writestring("kickout: missing username\n");
    } else if (strcmp(cmd, "govisual") == 0) {
        cmd_startgui();
    } else if (strcmp(cmd, "goblind") == 0) {
        cmd_exitgui();
    } else if (strcmp(cmd, "spawn") == 0) {
        char* title = strtok(NULL, " ");
        char* xs = strtok(NULL, " ");
        char* ys = strtok(NULL, " ");
        char* ws = strtok(NULL, " ");
        char* hs = strtok(NULL, " ");
        if (title && xs && ys && ws && hs) {
            // Simple atoi for parameters
            int x = 0, y = 0, w = 0, h = 0;
            for (int i = 0; xs[i] >= '0' && xs[i] <= '9'; i++) x = x * 10 + (xs[i] - '0');
            for (int i = 0; ys[i] >= '0' && ys[i] <= '9'; i++) y = y * 10 + (ys[i] - '0');
            for (int i = 0; ws[i] >= '0' && ws[i] <= '9'; i++) w = w * 10 + (ws[i] - '0');
            for (int i = 0; hs[i] >= '0' && hs[i] <= '9'; i++) h = h * 10 + (hs[i] - '0');
            cmd_newwin(title, x, y, w, h);
        } else {
            terminal_writestring("spawn: usage: spawn <title> <x> <y> <w> <h>\n");
        }
    } else if (strcmp(cmd, "kill") == 0) {
        char* id_str = strtok(NULL, " ");
        if (id_str) {
            int id = 0;
            for (int i = 0; id_str[i] >= '0' && id_str[i] <= '9'; i++) {
                id = id * 10 + (id_str[i] - '0');
            }
            cmd_closewin(id);
        } else {
            terminal_writestring("kill: missing window id\n");
        }
    } else if (strcmp(cmd, "focus") == 0) {
        char* id_str = strtok(NULL, " ");
        if (id_str) {
            int id = 0;
            for (int i = 0; id_str[i] >= '0' && id_str[i] <= '9'; i++) {
                id = id * 10 + (id_str[i] - '0');
            }
            cmd_focuswin(id);
        } else {
            terminal_writestring("focus: missing window id\n");
        }
    } else if (strcmp(cmd, "windows") == 0) {
        cmd_listwin();
    } else if (strcmp(cmd, "scribble") == 0) {
        char* id_str = strtok(NULL, " ");
        char* text = strtok(NULL, "");  // Get rest of line
        if (id_str && text) {
            int id = 0;
            for (int i = 0; id_str[i] >= '0' && id_str[i] <= '9'; i++) {
                id = id * 10 + (id_str[i] - '0');
            }
            cmd_writewin(id, text);
        } else {
            terminal_writestring("writewin: usage: writewin <id> <text>\n");
        }
    } else if (strcmp(cmd, "wipe") == 0) {
        clear_screen();
    } else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(cmd);
        terminal_writestring("\nType 'whatdo' for help\n");
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
    
    // Calibrate CPU speed for accurate timing
    terminal_writestring("  [..] Calibrating CPU speed...");
    calibrate_cpu_speed();
    terminal_writestring(" done\n");
    terminal_writestring("       TSC frequency: ");
    char speed_buf[16];
    // Convert KHz to MHz without 64-bit division
    uint32_t mhz = 0;
    uint64_t temp = tsc_freq_khz;
    while (temp >= 1000) {
        temp -= 1000;
        mhz++;
    }
    itoa_helper(mhz, speed_buf);
    terminal_writestring(speed_buf);
    terminal_writestring(" MHz\n");
    
    // Initialize subsystems
    init_fs();
    terminal_writestring("  [OK] Filesystem initialized\n");
    
    init_auth();
    terminal_writestring("  [OK] Authentication system ready\n");
    
    // Initialize network driver
    if (net_driver_init()) {
        // Get MAC address from driver
        net_driver_get_mac_address(net_device.mac);
        
        // Initialize IP to 0.0.0.0 (will be set by DHCP)
        memset(net_device.ip, 0, IP_ADDR_LEN);
        memset(net_device.gateway, 0, IP_ADDR_LEN);
        memset(net_device.netmask, 0, IP_ADDR_LEN);
        memset(net_device.dns, 0, IP_ADDR_LEN);
        net_device.dhcp_configured = false;
        net_device.initialized = true;
        
        // Initialize DNS cache
        dns_cache_init();
        
        // Show which driver was detected
        terminal_writestring("  [OK] Network card detected (");
        terminal_writestring(net_driver_get_name());
        terminal_writestring(")\n");
        
        // Wait for link to come up (important for VirtualBox)
        terminal_writestring("  [..] Waiting for network link...\n");
        delay_seconds(1);  // 1 second link stabilization delay
        
        // Try DHCP
        terminal_writestring("  [..] Requesting IP via DHCP...\n");
        if (dhcp_client_run()) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            terminal_writestring("  [OK] DHCP configuration successful\n");
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
            
            // Show assigned IP
            char buf[16];
            terminal_writestring("       IP: ");
            for (int i = 0; i < IP_ADDR_LEN; i++) {
                itoa_helper(net_device.ip[i], buf);
                terminal_writestring(buf);
                if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
            }
            terminal_writestring("\n");
        } else {
            terminal_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
            terminal_writestring("  [WARN] DHCP failed, no IP assigned\n");
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
            terminal_writestring("         Use 'dhcp' command to retry\n");
        }
    } else {
        terminal_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
        terminal_writestring("  [WARN] No network card found\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
    terminal_writestring("\n");
    
    // Login
    if (!login()) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("\nAuthentication failed. System halted.\n");
        while (1) __asm__ volatile ("hlt");
    }
    
    // Check DHCP status again after login (in case it completed during login)
    if (net_device.initialized && net_device.dhcp_configured) {
        // If DHCP wasn't configured during boot but is now, show success
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("\n[Network] DHCP configuration completed during login:\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        
        char buf[16];
        terminal_writestring("          IP: ");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(net_device.ip[i], buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
        }
        terminal_writestring("\n\n");
    }
    
    // Welcome message
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("slopOS ready. Keep it sloppy!\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("Type 'whatdo' for commands.\n\n");
    
    // Command loop
    char line[256];
    while (1) {
        // Poll network for incoming packets
        net_poll();
        
        // Check for TCP data when flop is active
        if (flop_conn_id >= 0 && tcp_is_established(flop_conn_id)) {
            uint8_t recv_buf[256];
            int received = tcp_receive_data(flop_conn_id, recv_buf, sizeof(recv_buf) - 1);
            if (received > 0) {
                recv_buf[received] = '\0';
                terminal_writestring((char*)recv_buf);
            }
        }
        
        // In GUI mode, just add a newline before prompt for separation
        // Don't clear or reposition - let output be visible
        if (gui_mode) {
            terminal_writestring("\n");
        }
        
        // Sloppy prompt style: [user vibing in /path]>
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_writestring("[");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_writestring(get_current_username());
        terminal_setcolor(vga_entry_color(VGA_COLOR_PINK, VGA_COLOR_BLACK));
        terminal_writestring(" vibing in ");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK));
        terminal_writestring(entries[cwd].name);
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_writestring("]");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("> ");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        
        // Read command
        read_line(line, sizeof(line));
        
        // Process command
        if (line[0] != '\0') {
            process_command(line);
        }
    }
}
