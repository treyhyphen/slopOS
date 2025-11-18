#include "../kernel_api.h"
#include "commands.h"
#include "../net_integration.h"

// This file contains all network-related commands
// Extracted from kernel.c to improve code organization

// External references to kernel data structures
extern NetworkDevice net_device;
extern PingState ping_state;

// External function declarations  
extern void arp_send_request(uint8_t* target_ip);
extern uint8_t* arp_lookup(uint8_t* ip);
extern bool dns_resolve(const char* name, uint8_t* ip);
extern void send_icmp_echo(uint8_t* dest_ip, uint8_t* dest_mac, uint16_t seq);
extern void send_icmp_request(uint8_t* dest_ip, uint8_t* dest_mac, uint16_t id, uint16_t seq);
extern bool is_local_subnet(uint8_t* ip);
extern bool parse_ip(const char* str, uint8_t* ip);
extern bool dhcp_client_run(void);
extern uint16_t htons(uint16_t n);
extern void delay_seconds(uint32_t seconds);

void cmd_ifconfig(void) {
    if (!net_device.initialized) {
        terminal_writestring("Network device not initialized\n");
        terminal_writestring("Make sure you're running in QEMU with: -netdev user,id=net0 -device rtl8139,netdev=net0\n");
        return;
    }
    
    char buf[16];
    
    terminal_writestring("eth0: ");
    terminal_writestring(net_driver_get_name());
    terminal_writestring("\n");
    
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
    
    // Parse IP address (or resolve hostname)
    uint8_t target_ip[IP_ADDR_LEN];
    bool resolved_from_dns = false;
    
    if (!parse_ip(ip_str, target_ip)) {
        // Not an IP address, try DNS resolution
        terminal_writestring("Resolving hostname ");
        terminal_writestring(ip_str);
        terminal_writestring("...\n");
        
        if (!dns_resolve(ip_str, target_ip)) {
            terminal_writestring("Failed to resolve hostname\n");
            return;
        }
        
        resolved_from_dns = true;
        terminal_writestring("Resolved to ");
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            char buf[4];
            itoa_helper(target_ip[i], buf);
            terminal_writestring(buf);
            if (i < 3) terminal_putchar('.');
        }
        terminal_putchar('\n');
    }
    
    // Determine next hop (local or via gateway)
    uint8_t* arp_target;
    bool is_local = is_local_subnet(target_ip);
    
    if (is_local) {
        terminal_writestring("Destination is on local network\n");
        arp_target = target_ip;
    } else {
        terminal_writestring("Destination is remote, routing via gateway ");
        // Show gateway IP
        char buf[16];
        for (int i = 0; i < IP_ADDR_LEN; i++) {
            itoa_helper(net_device.gateway[i], buf);
            terminal_writestring(buf);
            if (i < IP_ADDR_LEN - 1) terminal_putchar('.');
        }
        terminal_writestring("\n");
        
        if (net_device.gateway[0] == 0) {
            terminal_writestring("No gateway configured - cannot reach remote hosts\n");
            return;
        }
        arp_target = net_device.gateway;
    }
    
    // Check if next hop MAC is in ARP cache
    uint8_t* next_hop_mac = arp_lookup(arp_target);
    if (!next_hop_mac) {
        terminal_writestring("Resolving next hop MAC via ARP...\n");
        arp_send_request(arp_target);
        
        // Wait for ARP reply (3 second timeout)
        int arp_attempts = 0;
        while (!next_hop_mac && arp_attempts < 30) {
            if (interrupt_command) {
                terminal_writestring("Interrupted\n");
                return;
            }
            net_poll();
            next_hop_mac = arp_lookup(arp_target);
            if (next_hop_mac) break;
            delay_ms(100);  // 100ms delay per attempt = 3 seconds total
            arp_attempts++;
        }
        
        if (!next_hop_mac) {
            terminal_writestring("ARP timeout - next hop unreachable\n");
            return;
        }
    }
    
    // Initialize ping state
    // Note: target_ip is the final destination, but next_hop_mac is who we send to
    ping_state.active = true;
    ping_state.ping_id = 0x1234; // Fixed ID for simplicity
    ping_state.pings_sent = 0;
    ping_state.pings_received = 0;
    ping_state.ping_count = count;
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        ping_state.target_ip[i] = target_ip[i];
    }
    for (int i = 0; i < ETH_ALEN; i++) {
        ping_state.target_mac[i] = next_hop_mac[i];
    }
    
    terminal_writestring("PING ");
    terminal_writestring(ip_str);
    terminal_writestring(" (");
    terminal_writestring(ip_str);
    terminal_writestring(") sending ");
    char buf[16];
    itoa_helper(ping_state.ping_count, buf);
    terminal_writestring(buf);
    terminal_writestring(" packets:\n");
    
    // Send pings
    for (int i = 0; i < ping_state.ping_count; i++) {
        if (interrupt_command) {
            terminal_writestring("\nInterrupted\n");
            ping_state.active = false;
            return;
        }
        
        ping_state.ping_seq = htons(i + 1);
        ping_state.pings_sent++;
        
        terminal_writestring("  Sending ICMP echo request seq=");
        itoa_helper(i + 1, buf);
        terminal_writestring(buf);
        terminal_writestring("...\n");
        
        send_icmp_request(ping_state.target_ip, ping_state.target_mac, 
                         ping_state.ping_id, ping_state.ping_seq);
        
        // Wait for reply (1 second timeout)
        int wait_attempts = 0;
        int initial_received = ping_state.pings_received;
        while (ping_state.pings_received == initial_received && wait_attempts < 100) {
            if (interrupt_command) {
                terminal_writestring("\nInterrupted\n");
                ping_state.active = false;
                return;
            }
            net_poll();
            if (ping_state.pings_received > initial_received) break;
            delay_ms(10);  // 10ms delay = 1 second total timeout
            wait_attempts++;
        }
        
        if (ping_state.pings_received == initial_received) {
            terminal_writestring("  Timeout - no reply\n");
        }
        
        // Delay between pings (1 second)
        if (i < count - 1) {  // Don't delay after last ping
            delay_seconds(1);
        }
    }
    
    // Summary
    terminal_writestring("\n--- ");
    terminal_writestring(ip_str);
    terminal_writestring(" ping statistics ---\n");
    itoa_helper(ping_state.pings_sent, buf);
    terminal_writestring(buf);
    terminal_writestring(" packets transmitted, ");
    itoa_helper(ping_state.pings_received, buf);
    terminal_writestring(buf);
    terminal_writestring(" received, ");
    
    int loss = 100 - (ping_state.pings_received * 100 / ping_state.pings_sent);
    itoa_helper(loss, buf);
    terminal_writestring(buf);
    terminal_writestring("% packet loss\n");
    
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
