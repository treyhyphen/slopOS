#include "../kernel_api.h"

void cmd_help(void) {
    terminal_writestring("slopOS commands (keep it sloppy):\n");
    terminal_writestring("  peek             - list what's cookin\n");
    terminal_writestring("  cook <name>      - make a folder\n");
    terminal_writestring("  yeet <name>      - slap down a file\n");
    terminal_writestring("  bounce <name|..> - jump to folder\n");
    terminal_writestring("  whereami         - where am i at?\n");
    terminal_writestring("  me               - who am i?\n");
    if (is_current_user_admin()) {
        terminal_writestring("  whosthere        - see all users (admin)\n");
        terminal_writestring("  recruit          - add new user (admin)\n");
        terminal_writestring("  kickout          - boot a user (admin)\n");
    }
    terminal_writestring("  passwd           - change your secret\n");
    terminal_writestring("  mynet            - check your net setup\n");
    terminal_writestring("  gimmeip          - auto-grab an IP (DHCP)\n");
    terminal_writestring("  arp              - who's on the LAN\n");
    terminal_writestring("  netstat          - network stats\n");
    terminal_writestring("  route            - routing vibes\n");
    terminal_writestring("  poke <ip|host> [count] - poke a host (default: 5)\n");
    terminal_writestring("  wherez <host>    - find where a site lives\n");
    terminal_writestring("  flop <ip> <port> - connect to remote (netcat-style)\n");
    terminal_writestring("  flop -l <port>   - listen for connections\n");
    if (!gui_mode) {
        terminal_writestring("  govisual         - enter visual mode\n");
    } else {
        terminal_writestring("  spawn            - spawn a window\n");
        terminal_writestring("  kill             - yeet a window\n");
        terminal_writestring("  focus            - focus a window\n");
        terminal_writestring("  windows          - list windows\n");
        terminal_writestring("  scribble         - write to window\n");
        terminal_writestring("  goblind          - exit visual mode\n");
    }
    terminal_writestring("  wipe             - clear screen\n");
    terminal_writestring("  whatdo           - show this list\n");
}
