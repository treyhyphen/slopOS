#include "../kernel_api.h"

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
        terminal_writestring("  adduser        - add new user (admin)\n");
        terminal_writestring("  deluser        - delete user (admin)\n");
    }
    terminal_writestring("  passwd         - change password\n");
    terminal_writestring("  ifconfig       - show network configuration\n");
    terminal_writestring("  dhcp           - request IP via DHCP\n");
    terminal_writestring("  arp            - show ARP cache\n");
    terminal_writestring("  netstat        - show network statistics\n");
    terminal_writestring("  route          - show routing table\n");
    terminal_writestring("  ping <ip|host> [count] - send ICMP echo (default: 5 pings)\n");
    terminal_writestring("  nslookup <host> - resolve domain name via DNS\n");
    if (!gui_mode) {
        terminal_writestring("  startgui       - start GUI mode\n");
    } else {
        terminal_writestring("  newwin         - create new window\n");
        terminal_writestring("  closewin       - close window\n");
        terminal_writestring("  focuswin       - focus window\n");
        terminal_writestring("  listwin        - list windows\n");
        terminal_writestring("  writewin       - write to window\n");
        terminal_writestring("  exitgui        - exit GUI mode\n");
    }
    terminal_writestring("  clear          - clear screen\n");
    terminal_writestring("  help           - show this help\n");
}
