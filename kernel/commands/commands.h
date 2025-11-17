#ifndef COMMANDS_H
#define COMMANDS_H

#include "../types.h"

// Forward declarations for kernel functions that commands need
void terminal_writestring(const char* data);
void terminal_putchar(char c);
void terminal_setcolor(uint8_t color);
void itoa_helper(int n, char* buf);
void delay_ms(uint32_t ms);
void net_poll(void);

// VGA colors (from kernel.c)
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

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

// Filesystem commands
void cmd_help(void);
void cmd_ls(void);
void cmd_mkdir(const char* name);
void cmd_touch(const char* name);
void cmd_cd(const char* name);
void cmd_pwd(void);

// User/Auth commands
void cmd_whoami(void);
void cmd_listusers(void);
void cmd_passwd(void);
void cmd_adduser(const char* username, const char* password);
void cmd_deluser(const char* username);

// Network commands
void cmd_ifconfig(void);
void cmd_arp(void);
void cmd_netstat(void);
void cmd_route(void);
void cmd_ping(const char* ip_str, int count);
void cmd_nslookup(const char* hostname);
void cmd_dhcp(void);

// GUI commands
void cmd_startgui(void);
void cmd_exitgui(void);
void cmd_newwin(const char* title, int x, int y, int w, int h);
void cmd_closewin(int id);
void cmd_focuswin(int id);
void cmd_listwin(void);
void cmd_writewin(int id, const char* text);

#endif // COMMANDS_H
