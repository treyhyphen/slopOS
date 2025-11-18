#include "../kernel_api.h"

void cmd_startgui(void) {
    clear_screen();
    gui_mode = true;
    
    // Create a test window
    Window* win = &windows[window_count++];
    win->id = next_window_id++;
    strncpy(win->title, "Test Window", 63);
    win->x = 10;
    win->y = 5;
    win->width = 40;
    win->height = 10;
    win->visible = true;
    win->focused = true;
    
    // Add some visible text content
    const char* content = "Welcome to GUI mode!";
    strncpy(win->buffer, content, 1023);
    win->buffer_len = strlen(content);
    
    // Render the initial window
    render_all_windows();
}

void cmd_exitgui(void) {
    gui_mode = false;
    terminal_writestring("Exited GUI mode\n");
}

void cmd_newwin(const char* title, int x, int y, int w, int h) {
    if (window_count >= MAX_WINDOWS) {
        terminal_writestring("Maximum windows reached\n");
        return;
    }
    
    Window* win = &windows[window_count++];
    win->id = next_window_id++;
    strncpy(win->title, title, MAX_NAME - 1);
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->visible = true;
    win->focused = true;
    win->buffer[0] = '\0';
    win->buffer_len = 0;
    
    // Unfocus all other windows
    for (int i = 0; i < window_count - 1; i++) {
        windows[i].focused = false;
    }
    
    render_all_windows();
}

void cmd_closewin(int id) {
    for (int i = 0; i < window_count; i++) {
        if (windows[i].id == id) {
            // Shift windows down
            for (int j = i; j < window_count - 1; j++) {
                windows[j] = windows[j + 1];
            }
            window_count--;
            clear_screen();
            render_all_windows();
            terminal_writestring("Window closed\n");
            return;
        }
    }
    terminal_writestring("Window not found\n");
}

void cmd_focuswin(int id) {
    for (int i = 0; i < window_count; i++) {
        windows[i].focused = (windows[i].id == id);
        if (windows[i].id == id) {
            clear_screen();
            render_all_windows();
            terminal_writestring("Window ");
            char id_str[12];
            itoa_helper(id, id_str);
            terminal_writestring(id_str);
            terminal_writestring(" focused\n");
            return;
        }
    }
    terminal_writestring("Window not found\n");
}

void cmd_listwin(void) {
    if (window_count == 0) {
        terminal_writestring("No windows\n");
        return;
    }
    
    terminal_writestring("Windows:\n");
    for (int i = 0; i < window_count; i++) {
        terminal_writestring("  [");
        char buf[12];
        itoa_helper(windows[i].id, buf);
        terminal_writestring(buf);
        terminal_writestring("] ");
        terminal_writestring(windows[i].title);
        terminal_writestring(" (");
        itoa_helper(windows[i].width, buf);
        terminal_writestring(buf);
        terminal_writestring("x");
        itoa_helper(windows[i].height, buf);
        terminal_writestring(buf);
        terminal_writestring(" at ");
        itoa_helper(windows[i].x, buf);
        terminal_writestring(buf);
        terminal_writestring(",");
        itoa_helper(windows[i].y, buf);
        terminal_writestring(buf);
        terminal_writestring(")");
        if (windows[i].focused) terminal_writestring(" *focused*");
        terminal_writestring("\n");
    }
}

void cmd_writewin(int id, const char* text) {
    for (int i = 0; i < window_count; i++) {
        if (windows[i].id == id) {
            strncpy(windows[i].buffer, text, sizeof(windows[i].buffer) - 1);
            windows[i].buffer_len = strlen(windows[i].buffer);
            clear_screen();
            render_all_windows();
            terminal_writestring("Text written to window ");
            char id_str[12];
            itoa_helper(id, id_str);
            terminal_writestring(id_str);
            terminal_writestring("\n");
            return;
        }
    }
    terminal_writestring("Window not found\n");
}
