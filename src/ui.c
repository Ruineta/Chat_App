#include "../include/ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

// Terminal width (default, can be adjusted)
#define TERMINAL_WIDTH 80
#define BUBBLE_MAX_WIDTH 50
#define BUBBLE_INDENT_RIGHT 30  // Spaces from right for "me" messages

// Initialize UI (set UTF-8 encoding for Windows)
void ui_init(void) {
    #ifdef _WIN32
    // Enable UTF-8 for both output and input on Windows console
    SetConsoleOutputCP(65001); // Print UTF-8 (Tiếng Việt, Emoji)
    SetConsoleCP(65001);       // Read UTF-8 from keyboard
    // Enable ANSI escape codes (Windows 10+)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= 0x0004;  // ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004
        SetConsoleMode(hOut, dwMode);
    }
    #endif
}

// Clear screen (cross-platform)
void ui_clear_screen(void) {
    #ifdef _WIN32
    system("cls");
    #else
    system("clear");
    #endif
}

// Print header with box style (clears screen)
void ui_print_header(const char* title) {
    ui_clear_screen();
    
    int width = 70;
    int title_len = (int)strlen(title);
    int padding = (width - title_len - 2) / 2;
    
    // Top border
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_TL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s", BOX_TR);
    printf("%s\n", COLOR_RESET);
    
    // Title line
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    for (int i = 0; i < padding; i++) printf(" ");
    printf("%s", title);
    for (int i = 0; i < width - title_len - padding - 2; i++) printf(" ");
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    // Bottom border
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_BL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s", BOX_BR);
    printf("%s\n", COLOR_RESET);
}

// Print menu header without clearing screen (preserves notifications)
void ui_print_menu_header(const char* title) {
    printf("\n");  // Just add a newline, don't clear screen
    
    int width = 70;
    int title_len = (int)strlen(title);
    int padding = (width - title_len - 2) / 2;
    
    // Top border
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_TL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s", BOX_TR);
    printf("%s\n", COLOR_RESET);
    
    // Title line
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    for (int i = 0; i < padding; i++) printf(" ");
    printf("%s", title);
    for (int i = 0; i < width - title_len - padding - 2; i++) printf(" ");
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    // Bottom border
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_BL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s", BOX_BR);
    printf("%s\n", COLOR_RESET);
}

// Print navbar (web-style)
void ui_print_navbar(const char* title, const char* username) {
    int width = 70;
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_TL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s", BOX_TR);
    printf("%s\n", COLOR_RESET);
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    printf(" %s", title);
    int title_space = width - 2 - (int)strlen(title) - 1;
    if (username && strlen(username) > 0) {
        int user_len = (int)strlen(username);
        int space_before_user = title_space - user_len - 3;
        for (int i = 0; i < space_before_user; i++) printf(" ");
        printf(" %s%s%s ", COLOR_WHITE, username, COLOR_HEADER);
    } else {
        for (int i = 0; i < title_space; i++) printf(" ");
    }
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_BL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s", BOX_BR);
    printf("%s\n", COLOR_RESET);
}

// Print separator line
void ui_print_separator(void) {
    printf("%s", COLOR_INFO);
    for (int i = 0; i < 70; i++) printf("%s", BOX_H);
    printf("%s\n", COLOR_RESET);
}

// Print menu item with formatting
void ui_print_menu_item(int index, const char* label) {
    if (index == 0) {
        printf("%s  %s%-3d%s. %s\n", COLOR_MENU_TEXT, COLOR_INFO, index, COLOR_MENU_TEXT, label);
    } else {
        printf("  %s%-3d%s. %s\n", COLOR_INFO, index, COLOR_RESET, label);
    }
}

// Print notification with color
void ui_print_notification(const char* msg, NotificationType type) {
    const char* color;
    const char* icon;
    
    switch (type) {
        case NOTIF_SUCCESS:
            color = COLOR_SUCCESS;
            icon = "✓";
            break;
        case NOTIF_ERROR:
            color = COLOR_ERROR;
            icon = "✗";
            break;
        case NOTIF_INFO:
            color = COLOR_INFO;
            icon = "ℹ";
            break;
        case NOTIF_WARNING:
            color = COLOR_WARNING;
            icon = "⚠";
            break;
        default:
            color = COLOR_RESET;
            icon = "•";
    }
    
    printf("\n%s[%s]%s %s%s%s\n", color, icon, COLOR_RESET, color, msg, COLOR_RESET);
}

// Print input prompt
void ui_input_prompt(const char* label) {
    printf("%s> %s%s:%s ", COLOR_INFO, COLOR_MENU_TEXT, label, COLOR_RESET);
    fflush(stdout);
}

// Print box top border
void ui_print_box_top(int width) {
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_TL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s", BOX_TR);
    printf("%s\n", COLOR_RESET);
}

// Print box middle line with content
void ui_print_box_middle(int width, const char* content) {
    int content_len = (int)strlen(content);
    int padding = (width - content_len - 2) / 2;
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    for (int i = 0; i < padding; i++) printf(" ");
    printf("%s", content);
    for (int i = 0; i < width - content_len - padding - 2; i++) printf(" ");
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
}

// Print box bottom border
void ui_print_box_bottom(int width) {
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_BL);
    for (int i = 0; i < width - 2; i++) printf("%s", BOX_H);
    printf("%s", BOX_BR);
    printf("%s\n", COLOR_RESET);
}

// Print section header
void ui_print_section_header(const char* title) {
    printf("\n%s%s %s %s\n", COLOR_INFO, BOX_C, title, COLOR_RESET);
    ui_print_separator();
}

// Clear the current input/prompt line
void ui_clear_prompt_line(void) {
    // Move cursor to line start, overwrite with spaces, move back.
    printf("\r");
    for (int i = 0; i < TERMINAL_WIDTH; i++) {
        printf(" ");
    }
    printf("\r");
    fflush(stdout);
}

// Draw message bubble (core function) - OLD VERSION, kept for compatibility
void ui_draw_bubble(const char* content, int align_right, const char* color, int max_width) {
    int content_len = (int)strlen(content);
    if (content_len == 0) return;
    
    // Calculate actual bubble width (content + padding)
    int bubble_width = (content_len < max_width) ? content_len + 4 : max_width + 4;
    
    // Calculate padding for alignment
    int left_padding = 0;
    if (align_right) {
        left_padding = TERMINAL_WIDTH - bubble_width - BUBBLE_INDENT_RIGHT;
        if (left_padding < 0) left_padding = 0;
    }
    
    // Word wrap: split content into lines
    char lines[10][256];  // Max 10 lines
    int line_count = 0;
    int pos = 0;
    
    while (pos < content_len && line_count < 10) {
        int remaining = content_len - pos;
        int line_len = (remaining < max_width) ? remaining : max_width;
        
        // Try to break at space if possible
        if (line_len < remaining && content[pos + line_len] != ' ') {
            int break_pos = line_len;
            while (break_pos > 0 && content[pos + break_pos] != ' ') {
                break_pos--;
            }
            if (break_pos > max_width * 0.7) {  // Only break if not too short
                line_len = break_pos;
            }
        }
        
        strncpy(lines[line_count], content + pos, line_len);
        lines[line_count][line_len] = '\0';
        line_count++;
        
        pos += line_len;
        // Skip space if we broke at word boundary
        if (pos < content_len && content[pos] == ' ') pos++;
    }
    
    // Find max line width for consistent bubble
    int max_line_width = 0;
    for (int i = 0; i < line_count; i++) {
        int len = (int)strlen(lines[i]);
        if (len > max_line_width) max_line_width = len;
    }
    bubble_width = max_line_width + 4;
    
    // Recalculate padding
    if (align_right) {
        left_padding = TERMINAL_WIDTH - bubble_width - BUBBLE_INDENT_RIGHT;
        if (left_padding < 0) left_padding = 0;
    }
    
    // Print top border
    for (int i = 0; i < left_padding; i++) printf(" ");
    printf("%s%s", color, BUBBLE_TL);
    for (int i = 0; i < bubble_width - 2; i++) printf("%s", BUBBLE_H);
    printf("%s%s\n", BUBBLE_TR, COLOR_RESET);
    
    // Print content lines
    for (int i = 0; i < line_count; i++) {
        for (int j = 0; j < left_padding; j++) printf(" ");
        printf("%s%s", color, BUBBLE_V);
        printf(" %s", lines[i]);
        int spaces = bubble_width - 2 - (int)strlen(lines[i]) - 1;
        for (int j = 0; j < spaces; j++) printf(" ");
        printf(" %s%s\n", BUBBLE_V, COLOR_RESET);
    }
    
    // Print bottom border
    for (int i = 0; i < left_padding; i++) printf(" ");
    printf("%s%s", color, BUBBLE_BL);
    for (int i = 0; i < bubble_width - 2; i++) printf("%s", BUBBLE_H);
    printf("%s%s\n", BUBBLE_BR, COLOR_RESET);
}

// Print modern chat bubble (used in chat room & history)
void ui_print_message_bubble_simple(const char* sender, const char* content, int is_me, const char* timestamp, const char* prompt_username) {
    if (!content || strlen(content) == 0) return;
    
    const int max_width = 48;
    const char* bubble_color = is_me ? COLOR_ME : COLOR_FRIEND;
    const char* header_color = COLOR_SYSTEM;
    int align_right = is_me;
    
    ui_clear_prompt_line();
    
    // Prepare header line (sender + timestamp)
    char header[128];
    if (timestamp && strlen(timestamp) > 0) {
        snprintf(header, sizeof(header), "%s  •  %s", sender, timestamp);
    } else {
        snprintf(header, sizeof(header), "%s", sender);
    }
    
    // Word wrapping
    int content_len = (int)strlen(content);
    char lines[16][256];
    int line_count = 0;
    int pos = 0;
    
    while (pos < content_len && line_count < 16) {
        int remaining = content_len - pos;
        int line_len = remaining < max_width ? remaining : max_width;
        
        if (line_len < remaining && content[pos + line_len] != ' ') {
            int break_pos = line_len;
            while (break_pos > 0 && content[pos + break_pos] != ' ') {
                break_pos--;
            }
            if (break_pos > max_width * 0.5) {
                line_len = break_pos;
            }
        }
        
        strncpy(lines[line_count], content + pos, line_len);
        lines[line_count][line_len] = '\0';
        line_count++;
        
        pos += line_len;
        if (pos < content_len && content[pos] == ' ') pos++;
    }
    
    int max_line_width = (int)strlen(header);
    for (int i = 0; i < line_count; i++) {
        int len = (int)strlen(lines[i]);
        if (len > max_line_width) max_line_width = len;
    }
    
    int bubble_width = max_line_width + 4;
    if (bubble_width > TERMINAL_WIDTH - 4) {
        bubble_width = TERMINAL_WIDTH - 4;
    }
    int left_padding = align_right ? TERMINAL_WIDTH - bubble_width - BUBBLE_INDENT_RIGHT : 2;
    if (left_padding < 0) left_padding = 0;
    
    // Top border
    for (int i = 0; i < left_padding; i++) printf(" ");
    printf("%s%s", bubble_color, BUBBLE_TL);
    for (int i = 0; i < bubble_width - 2; i++) printf("%s", BUBBLE_H);
    printf("%s%s\n", BUBBLE_TR, COLOR_RESET);
    
    // Header line
    int header_len = (int)strlen(header);
    if (header_len > bubble_width - 4) {
        header_len = bubble_width - 4;
    }
    for (int i = 0; i < left_padding; i++) printf(" ");
    printf("%s%s %s", bubble_color, BUBBLE_V, header_color);
    fwrite(header, 1, header_len, stdout);
    printf("%s", COLOR_RESET);
    int header_spaces = bubble_width - 3 - header_len;
    for (int i = 0; i < header_spaces; i++) printf(" ");
    printf("%s%s\n", bubble_color, BUBBLE_V);
    printf("%s", COLOR_RESET);
    
    // Separator line inside bubble
    for (int i = 0; i < left_padding; i++) printf(" ");
    printf("%s%s", bubble_color, BUBBLE_V);
    for (int i = 0; i < bubble_width - 2; i++) printf("%s", " ");
    printf("%s%s\n", bubble_color, BUBBLE_V);
    printf("%s", COLOR_RESET);
    
    // Content lines
    for (int i = 0; i < line_count; i++) {
        int visible_len = (int)strlen(lines[i]);
        if (visible_len > bubble_width - 4) {
            lines[i][bubble_width - 4] = '\0';
            visible_len = bubble_width - 4;
        }
        
        for (int j = 0; j < left_padding; j++) printf(" ");
        printf("%s%s ", bubble_color, BUBBLE_V);
        printf("%s", COLOR_RESET);
        printf("%s", lines[i]);
        int spaces = bubble_width - 3 - visible_len;
        for (int s = 0; s < spaces; s++) printf(" ");
        printf("%s%s\n", bubble_color, BUBBLE_V);
        printf("%s", COLOR_RESET);
    }
    
    // Bottom border
    for (int i = 0; i < left_padding; i++) printf(" ");
    printf("%s%s", bubble_color, BUBBLE_BL);
    for (int i = 0; i < bubble_width - 2; i++) printf("%s", BUBBLE_H);
    printf("%s%s\n\n", BUBBLE_BR, COLOR_RESET);

    if (prompt_username && prompt_username[0] != '\0') {
        ui_print_chat_prompt(prompt_username);
    }
}

// Print message bubble (web-style chat bubble) - OLD VERSION
void ui_print_message_bubble(const char* sender, const char* content, int is_me, const char* timestamp) {
    const char* color = is_me ? COLOR_ME : COLOR_FRIEND;
    const char* name_color = is_me ? COLOR_ME : COLOR_FRIEND;
    
    // Print timestamp if provided (left aligned)
    if (timestamp) {
        printf("%s[%s]%s ", COLOR_SYSTEM, timestamp, COLOR_RESET);
    }
    
    // Draw bubble
    ui_draw_bubble(content, is_me, color, BUBBLE_MAX_WIDTH);
    
    // Print sender name below bubble (aligned with bubble)
    int content_len = (int)strlen(content);
    int bubble_width = (content_len < BUBBLE_MAX_WIDTH) ? content_len + 4 : BUBBLE_MAX_WIDTH + 4;
    int name_padding = 0;
    
    if (is_me) {
        name_padding = TERMINAL_WIDTH - bubble_width - BUBBLE_INDENT_RIGHT - (int)strlen(sender);
        if (name_padding < 0) name_padding = 0;
    } else {
        name_padding = 0;  // Left aligned
    }
    
    for (int i = 0; i < name_padding; i++) printf(" ");
    printf("%s%s%s\n", name_color, sender, COLOR_RESET);
    printf("\n");
}

// Print chat room header
void ui_print_chat_header(const char* title) {
    ui_clear_screen();
    
    printf("%s", COLOR_HEADER);
    printf("╔══ CHAT ROOM: %s ══╗\n", title);
    printf("%s", COLOR_RESET);
    printf("\n");
}

// Print chat prompt
void ui_print_chat_prompt(const char* username) {
    ui_clear_prompt_line();
    printf("%s[%s]:%s ", COLOR_ME, username, COLOR_RESET);
    fflush(stdout);
}

// Print fancy menu (card/dashboard style)
void ui_print_menu_fancy(void) {
    printf("\n");
    
    // Top border
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_TL);
    for (int i = 0; i < 68; i++) printf("%s", BOX_H);
    printf("%s", BOX_TR);
    printf("%s\n", COLOR_RESET);
    
    // Title
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    printf("  %sChat Application Dashboard%s", COLOR_WHITE, COLOR_HEADER);
    for (int i = 0; i < 68 - 2 - 25; i++) printf(" ");
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    // Separator
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_C);
    for (int i = 0; i < 68; i++) printf("%s", BOX_H);
    printf("%s", BOX_CR);
    printf("%s\n", COLOR_RESET);
    
    // Menu items in card style
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    printf("  %sAccount & Social%s", COLOR_INFO, COLOR_RESET);
    for (int i = 0; i < 68 - 2 - 17; i++) printf(" ");
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    printf("    %s1-10%s: Register, Login, Friends, Requests, Block...", COLOR_MENU_TEXT, COLOR_RESET);
    for (int i = 0; i < 68 - 2 - 50; i++) printf(" ");
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_C);
    for (int i = 0; i < 68; i++) printf("%s", BOX_H);
    printf("%s", BOX_CR);
    printf("%s\n", COLOR_RESET);
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    printf("  %sChatting%s", COLOR_INFO, COLOR_RESET);
    for (int i = 0; i < 68 - 2 - 9; i++) printf(" ");
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    printf("    %s11-20%s: Messages, Groups, History, Pin...", COLOR_MENU_TEXT, COLOR_RESET);
    for (int i = 0; i < 68 - 2 - 42; i++) printf(" ");
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_C);
    for (int i = 0; i < 68; i++) printf("%s", BOX_H);
    printf("%s", BOX_CR);
    printf("%s\n", COLOR_RESET);
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    printf("  %sSystem%s", COLOR_INFO, COLOR_RESET);
    for (int i = 0; i < 68 - 2 - 8; i++) printf(" ");
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_V);
    printf("    %s21%s: Disconnect  |  %s0%s: Exit", COLOR_MENU_TEXT, COLOR_RESET, COLOR_MENU_TEXT, COLOR_RESET);
    for (int i = 0; i < 68 - 2 - 35; i++) printf(" ");
    printf("%s", BOX_V);
    printf("%s\n", COLOR_RESET);
    
    // Bottom border
    printf("%s", COLOR_HEADER);
    printf("%s", BOX_BL);
    for (int i = 0; i < 68; i++) printf("%s", BOX_H);
    printf("%s", BOX_BR);
    printf("%s\n", COLOR_RESET);
}
