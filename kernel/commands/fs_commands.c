#include "../kernel_api.h"

void cmd_ls(void) {
    int i = entries[cwd].first_child;
    if (i == -1) {
        terminal_writestring("(empty directory)\n");
        return;
    }
    
    while (i != -1) {
        if (interrupt_command) {
            terminal_writestring("\n");
            return;
        }
        
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

void cmd_pwd_recursive(int idx, int depth) {
    if (depth > 100) {
        terminal_writestring("[ERROR: recursion limit]");
        return;
    }
    if (idx < 0 || idx >= MAX_ENTRIES) {
        terminal_writestring("[ERROR: invalid index]");
        return;
    }
    if (entries[idx].parent != -1) {
        cmd_pwd_recursive(entries[idx].parent, depth + 1);
        if (idx != 0) {
            terminal_writestring("/");
            terminal_writestring(entries[idx].name);
        }
    } else {
        terminal_writestring("/");
    }
}

void cmd_pwd(void) {
    cmd_pwd_recursive(cwd, 0);
    terminal_writestring("\n");
}
