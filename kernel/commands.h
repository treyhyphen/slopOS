#ifndef COMMANDS_H
#define COMMANDS_H

// Help command
void cmd_help(void);

// Filesystem commands
void cmd_ls(void);
void cmd_mkdir(const char* name);
void cmd_touch(const char* name);
void cmd_cd(const char* name);
void cmd_pwd(void);

// User commands
void cmd_whoami(void);
void cmd_listusers(void);
void cmd_passwd(void);
void cmd_adduser(const char* username, const char* password);
void cmd_deluser(const char* username);

// GUI commands
void cmd_startgui(void);
void cmd_exitgui(void);
void cmd_newwin(const char* title, int x, int y, int w, int h);
void cmd_closewin(int id);
void cmd_focuswin(int id);
void cmd_listwin(void);
void cmd_writewin(int id, const char* text);

// Network commands (in kernel.c)
void cmd_ifconfig(void);
void cmd_arp(void);
void cmd_netstat(void);
void cmd_route(void);
void cmd_ping(const char* ip_str, int count);
void cmd_nslookup(const char* hostname);
void cmd_dhcp(void);

#endif
