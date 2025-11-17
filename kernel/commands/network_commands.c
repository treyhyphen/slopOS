#include "../kernel_api.h"#include "commands.h"

#include "../net_integration.h"

// This file contains all network-related commands

// Extracted from kernel.c to improve code organization// External references to kernel data structures

extern NetworkDevice net_device;

void cmd_ifconfig(void);extern PingState ping_state;

void cmd_arp(void);

void cmd_netstat(void);// External function declarations

void cmd_route(void);extern void arp_send_request(uint8_t* target_ip);

void cmd_ping(const char* ip_str, int count);extern uint8_t* arp_lookup(uint8_t* ip);

void cmd_nslookup(const char* hostname);extern bool dns_resolve(const char* name, uint8_t* ip);

void cmd_dhcp(void);extern void send_icmp_echo(uint8_t* dest_ip, uint8_t* dest_mac, uint16_t seq);

extern bool is_local_subnet(uint8_t* ip);

// Network commands will be compiled in from kernel.c for nowextern bool parse_ip(const char* str, uint8_t* ip);

// Full extraction requires more refactoring of internal network structuresextern void dhcp_client_run(void);


void cmd_ifconfig(void) {
    if (!net_device.initialized) {
        terminal_writestring("Network device not initialized\n");
        terminal_writestring("Make sure you're running in QEMU with: -netdev user,id=net0 -device rtl8139,netdev=net0\n");
        return;
    }
    
    char buf[16];
    
    terminal_writestring("eth0: RTL8139 Fast Ethernet\n");
    
    // DHCP status
    if (net_device.dhcp_configured) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("  [DHCP configured]\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    } else if (net_device.ip[0] != 0) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
        terminal_writestring("  [Static IP]\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    } else {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_writestring("  [No IP address]\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
    
    terminal_writestring("  HWaddr: ");
    for (int i = 0; i < ETH_ALEN; i++) {
        char hex[] = "0123456789ABCDEF";
        terminal_putchar(hex[net_device.mac[i] >> 4]);
        terminal_putchar(hex[net_device.mac[i] & 0x0F]);
        if (i < ETH_ALEN - 1) terminal_putchar(':');
    }
    terminal_writestring("\n");
    
    if (net_device.ip[0] != 0) {
        terminal_writestring("  inet addr: ");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(net_device.ip[i], buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
        }
        terminal_writestring("\n");
    }
    
    if (net_device.gateway[0] != 0) {
        terminal_writestring("  Gateway: ");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(net_device.gateway[i], buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
        }
        terminal_writestring("\n");
    }
    
    if (net_device.netmask[0] != 0) {
        terminal_writestring("  Netmask: ");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(net_device.netmask[i], buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
        }
        terminal_writestring("\n");
    }
    
    if (net_device.dns[0] != 0) {
        terminal_writestring("  DNS: ");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(net_device.dns[i], buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
        }
        terminal_writestring("\n");
    }
}

void cmd_arp(void) {
    if (!net_device.initialized) {
        terminal_writestring("Network not initialized\n");
        return;
    }
    
    char buf[16];
    terminal_writestring("ARP cache:\n");
    terminal_writestring("IP Address       HW Address\n");
    
    int count = 0;
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (net_device.arp_cache[i].valid) {
            // Print IP
            for (int j = 0; j < IP_ADDR_LEN; j++) {
                itoa_helper(net_device.arp_cache[i].ip[j], buf);
                terminal_writestring(buf);
                if (j < IP_ADDR_LEN - 1) terminal_putchar('.');
            }
            
            // Padding
            terminal_writestring("     ");
            
            // Print MAC
            for (int j = 0; j < ETH_ALEN; j++) {
                char hex[] = "0123456789ABCDEF";
                terminal_putchar(hex[net_device.arp_cache[i].mac[j] >> 4]);
                terminal_putchar(hex[net_device.arp_cache[i].mac[j] & 0x0F]);
                if (j < ETH_ALEN - 1) terminal_putchar(':');
            }
            terminal_writestring("\n");
            count++;
        }
    }
    
    if (count == 0) {
        terminal_writestring("(empty)\n");
    } else {
        terminal_writestring("\nTotal entries: ");
        itoa_helper(count, buf);
        terminal_writestring(buf);
        terminal_writestring("\n");
    }
}

void cmd_netstat(void) {
    if (!net_device.initialized) {
        terminal_writestring("Network not initialized\n");
        return;
    }
    
    terminal_writestring("Active Network Connections:\n");
    terminal_writestring("Proto  Local Address          State\n");
    
    // DHCP
    if (net_device.dhcp_configured) {
        terminal_writestring("UDP    0.0.0.0:68             DHCP CLIENT\n");
    }
    
    // Ping
    if (ping_state.active) {
        terminal_writestring("ICMP   ");
        char buf[4];
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(net_device.ip[i], buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
        }
        terminal_writestring("            PING ACTIVE\n");
    }
    
    terminal_writestring("\n");
}

void cmd_route(void) {
    if (!net_device.initialized) {
        terminal_writestring("Network not initialized\n");
        return;
    }
    
    char buf[16];
    terminal_writestring("Kernel IP routing table\n");
    terminal_writestring("Destination      Gateway          Netmask          Iface\n");
    
    // Local network route
    if (net_device.ip[0] != 0 && net_device.netmask[0] != 0) {
        // Calculate network address
        terminal_writestring("");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            uint8_t net_addr = net_device.ip[i] & net_device.netmask[i];
            itoa_helper(net_addr, buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
            else for (int s = 0; s < (17 - (i+1)*4); s++) terminal_putchar(' ');
        }
        terminal_writestring("*                ");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(net_device.netmask[i], buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
            else for (int s = 0; s < (17 - (i+1)*4); s++) terminal_putchar(' ');
        }
        terminal_writestring("eth0\n");
    }
    
    // Default route
    if (net_device.gateway[0] != 0) {
        terminal_writestring("0.0.0.0          ");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(net_device.gateway[i], buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
            else for (int s = 0; s < (17 - (i+1)*4); s++) terminal_putchar(' ');
        }
        terminal_writestring("0.0.0.0          eth0\n");
    }
}

void cmd_ping(const char* ip_str, int count) {
    if (!net_device.initialized) {
        terminal_writestring("Network not initialized\n");
        return;
    }
    
    if (net_device.ip[0] == 0) {
        terminal_writestring("No IP address configured. Run 'dhcp' first.\n");
        return;
    }
    
    uint8_t dest_ip[IP_ADDR_LEN];
    bool resolved_from_dns = false;
    
    // Try to parse as IP first
    if (!parse_ip(ip_str, dest_ip)) {
        // Not an IP, try DNS resolution
        terminal_writestring("Resolving ");
        terminal_writestring(ip_str);
        terminal_writestring("...\n");
        
        if (!dns_resolve(ip_str, dest_ip)) {
            terminal_writestring("ping: unknown host ");
            terminal_writestring(ip_str);
            terminal_writestring("\n");
            return;
        }
        resolved_from_dns = true;
    }
    
    // Determine next hop
    uint8_t* arp_target;
    if (is_local_subnet(dest_ip)) {
        arp_target = dest_ip;
    } else {
        if (net_device.gateway[0] == 0) {
            terminal_writestring("No gateway configured\n");
            return;
        }
        arp_target = net_device.gateway;
    }
    
    // Resolve MAC address
    uint8_t* dest_mac = arp_lookup(arp_target);
    if (!dest_mac) {
        terminal_writestring("Resolving MAC address...\n");
        arp_send_request(arp_target);
        
        // Wait for ARP reply
        int arp_attempts = 0;
        while (!dest_mac && arp_attempts < 30) {
            net_poll();
            dest_mac = arp_lookup(arp_target);
            if (dest_mac) break;
            delay_ms(100);
            arp_attempts++;
        }
        
        if (!dest_mac) {
            terminal_writestring("Cannot reach destination (ARP timeout)\n");
            return;
        }
    }
    
    // Start ping
    ping_state.active = true;
    ping_state.count = count;
    ping_state.received = 0;
    ping_state.sequence = 0;
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        ping_state.target_ip[i] = dest_ip[i];
    }
    
    char buf[16];
    terminal_writestring("PING ");
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        itoa_helper(dest_ip[i], buf);
        terminal_writestring(buf);
        if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
    }
    terminal_writestring(": ");
    itoa_helper(count, buf);
    terminal_writestring(buf);
    terminal_writestring(" data bytes\n");
    
    // Send pings
    for (int i = 0; i < count; i++) {
        ping_state.sequence = i;
        ping_state.waiting = true;
        send_icmp_echo(dest_ip, dest_mac, i);
        
        // Wait for reply (1 second timeout)
        int attempts = 0;
        while (ping_state.waiting && attempts < 100) {
            net_poll();
            delay_ms(10);
            attempts++;
        }
        
        if (ping_state.waiting) {
            terminal_writestring("Request timeout for icmp_seq ");
            itoa_helper(i, buf);
            terminal_writestring(buf);
            terminal_writestring("\n");
        }
        
        // Wait 1 second between pings
        if (i < count - 1) {
            delay_ms(1000);
        }
    }
    
    // Summary
    terminal_writestring("\n--- ping statistics ---\n");
    itoa_helper(count, buf);
    terminal_writestring(buf);
    terminal_writestring(" packets transmitted, ");
    itoa_helper(ping_state.received, buf);
    terminal_writestring(buf);
    terminal_writestring(" packets received\n");
    
    ping_state.active = false;
}

void cmd_nslookup(const char* hostname) {
    if (!net_device.initialized) {
        terminal_writestring("Network not initialized\n");
        return;
    }
    
    if (net_device.dns[0] == 0) {
        terminal_writestring("No DNS server configured. Run 'dhcp' first.\n");
        return;
    }
    
    uint8_t ip[IP_ADDR_LEN];
    terminal_writestring("Server: ");
    char buf[4];
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        itoa_helper(net_device.dns[i], buf);
        terminal_writestring(buf);
        if (i < 3) terminal_putchar('.');
    }
    terminal_writestring("\n\n");
    
    if (dns_resolve(hostname, ip)) {
        terminal_writestring("Name:   ");
        terminal_writestring(hostname);
        terminal_writestring("\nAddress: ");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(ip[i], buf);
            terminal_writestring(buf);
            if (i < 3) terminal_putchar('.');
        }
        terminal_writestring("\n");
    } else {
        terminal_writestring("** server can't find ");
        terminal_writestring(hostname);
        terminal_writestring(": NXDOMAIN\n");
    }
}

void cmd_dhcp(void) {
    if (!net_device.initialized) {
        terminal_writestring("Network device not initialized\n");
        return;
    }
    
    if (net_device.dhcp_configured) {
        terminal_writestring("DHCP is already configured. Current settings:\n");
        cmd_ifconfig();
        terminal_writestring("\nRun 'dhcp' again to renew lease.\n");
    }
    
    terminal_writestring("Starting DHCP client...\n");
    dhcp_client_run();
    
    if (net_device.dhcp_configured) {
        terminal_writestring("\nDHCP configuration successful!\n");
        cmd_ifconfig();
    } else {
        terminal_writestring("\nDHCP configuration failed.\n");
    }
}
