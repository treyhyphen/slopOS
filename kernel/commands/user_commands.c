#include "../kernel_api.h"

void cmd_whoami(void) {
    terminal_writestring(get_current_username());
    if (is_current_user_admin()) {
        terminal_writestring(" (administrator)");
    }
    terminal_writestring("\n");
}

void cmd_listusers(void) {
    if (!is_current_user_admin()) {
        terminal_writestring("Permission denied\n");
        return;
    }
    
    terminal_writestring("Active users:\n");
    for (int i = 0; i < auth.user_count; i++) {
        if (auth.users[i].active) {
            terminal_writestring("  ");
            terminal_writestring(auth.users[i].username);
            terminal_writestring(auth.users[i].is_admin ? " (admin)" : " (user)");
            if (i == auth.current_user)
                terminal_writestring(" [current]");
            terminal_writestring("\n");
        }
    }
}

void cmd_passwd(void) {
    char oldpass[MAX_PASSWORD];
    char newpass[MAX_PASSWORD];
    char confirm[MAX_PASSWORD];
    
    terminal_writestring("Old password: ");
    read_line(oldpass, sizeof(oldpass));
    
    if (!verify_password(auth.current_user, oldpass)) {
        terminal_writestring("Incorrect password\n");
        memset(oldpass, 0, sizeof(oldpass));
        return;
    }
    
    terminal_writestring("New password: ");
    read_line(newpass, sizeof(newpass));
    
    terminal_writestring("Confirm password: ");
    read_line(confirm, sizeof(confirm));
    
    if (strcmp(newpass, confirm) != 0) {
        terminal_writestring("Passwords do not match\n");
        memset(oldpass, 0, sizeof(oldpass));
        memset(newpass, 0, sizeof(newpass));
        memset(confirm, 0, sizeof(confirm));
        return;
    }
    
    sha256_simple(newpass, auth.users[auth.current_user].password_hash);
    terminal_writestring("Password changed successfully\n");
    
    memset(oldpass, 0, sizeof(oldpass));
    memset(newpass, 0, sizeof(newpass));
    memset(confirm, 0, sizeof(confirm));
}

void cmd_adduser(const char* username, const char* password) {
    if (!is_current_user_admin()) {
        terminal_writestring("Permission denied\n");
        return;
    }
    
    if (auth.user_count >= MAX_USERS) {
        terminal_writestring("Maximum users reached\n");
        return;
    }
    
    if (find_user(username) != -1) {
        terminal_writestring("User already exists\n");
        return;
    }
    
    User* new_user = &auth.users[auth.user_count];
    strncpy(new_user->username, username, MAX_NAME - 1);
    sha256_simple(password, new_user->password_hash);
    new_user->is_admin = false;
    new_user->active = true;
    auth.user_count++;
    
    terminal_writestring("User created successfully\n");
}

void cmd_deluser(const char* username) {
    if (!is_current_user_admin()) {
        terminal_writestring("Permission denied\n");
        return;
    }
    
    if (strcmp(username, get_current_username()) == 0) {
        terminal_writestring("Cannot delete current user\n");
        return;
    }
    
    int user_idx = find_user(username);
    if (user_idx == -1) {
        terminal_writestring("User not found\n");
        return;
    }
    
    auth.users[user_idx].active = false;
    terminal_writestring("User deleted successfully\n");
}
