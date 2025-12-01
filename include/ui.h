#ifndef UI_H
#define UI_H

#include "common.h"

// Color Palette (ANSI codes - cross-platform)
#ifdef _WIN32
    // Windows: Enable ANSI escape codes (Windows 10+)
    // Use explicit ESC sequence with \x1b to avoid printing raw "32m" artifacts
    #define COLOR_RESET      "\x1b[0m"
    #define COLOR_SUCCESS    "\x1b[32m"   // Green
    #define COLOR_ERROR      "\x1b[31m"   // Red
    #define COLOR_INFO       "\x1b[36m"   // Cyan
    #define COLOR_WARNING    "\x1b[33m"   // Yellow
    #define COLOR_MENU_TEXT  "\x1b[1m"    // Bold
    #define COLOR_HEADER     "\x1b[1;36m" // Bold Cyan
    #define COLOR_ME         "\x1b[1;32m" // Bold Green (My messages)
    #define COLOR_FRIEND     "\x1b[1;37m" // Bright White (Friend messages)
    #define COLOR_SYSTEM     "\x1b[90m"   // Grey (System messages)
    #define COLOR_WHITE      "\x1b[37m"   // White
    #define COLOR_BG_ME      "\x1b[46m"   // Cyan background
    #define COLOR_BG_FRIEND  "\x1b[43m"   // Yellow background
#else
    // Linux/Unix: Native ANSI support
    #define COLOR_RESET      "\x1b[0m"
    #define COLOR_SUCCESS    "\x1b[32m"
    #define COLOR_ERROR      "\x1b[31m"
    #define COLOR_INFO       "\x1b[36m"
    #define COLOR_WARNING    "\x1b[33m"
    #define COLOR_MENU_TEXT  "\x1b[1m"
    #define COLOR_HEADER     "\x1b[1;36m"
    #define COLOR_ME         "\x1b[1;32m"
    #define COLOR_FRIEND     "\x1b[1;37m"
    #define COLOR_SYSTEM     "\x1b[90m"
    #define COLOR_WHITE      "\x1b[37m"
    #define COLOR_BG_ME      "\x1b[46m"
    #define COLOR_BG_FRIEND  "\x1b[43m"
#endif

// Box drawing characters (UTF-8) - Rounded style for modern look
#define BUBBLE_TL "╭"  // Top Left (rounded)
#define BUBBLE_TR "╮"  // Top Right (rounded)
#define BUBBLE_BL "╰"  // Bottom Left (rounded)
#define BUBBLE_BR "╯"  // Bottom Right (rounded)
#define BUBBLE_H  "─"  // Horizontal
#define BUBBLE_V  "│"  // Vertical

// Double border for headers/menus
#define BOX_TL "╔"  // Top Left
#define BOX_TR "╗"  // Top Right
#define BOX_BL "╚"  // Bottom Left
#define BOX_BR "╝"  // Bottom Right
#define BOX_H  "═"  // Horizontal
#define BOX_V  "║"  // Vertical
#define BOX_C  "╠"  // Center Left
#define BOX_CR "╣"  // Center Right
#define BOX_CT "╦"  // Center Top
#define BOX_CB "╩"  // Center Bottom

// Notification types
typedef enum {
    NOTIF_SUCCESS = 0,
    NOTIF_ERROR = 1,
    NOTIF_INFO = 2,
    NOTIF_WARNING = 3
} NotificationType;

// Function declarations
void ui_init(void);
void ui_clear_screen(void);
void ui_print_header(const char* title);
void ui_print_menu_header(const char* title);  // Menu header without clearing screen
void ui_print_separator(void);
void ui_print_menu_item(int index, const char* label);
void ui_print_notification(const char* msg, NotificationType type);
void ui_input_prompt(const char* label);
void ui_print_box_top(int width);
void ui_print_box_middle(int width, const char* content);
void ui_print_box_bottom(int width);
void ui_print_section_header(const char* title);

// New web-style functions
void ui_print_message_bubble_simple(const char* sender, const char* content, int is_me, const char* timestamp, const char* prompt_username);
void ui_print_menu_fancy(void);
void ui_print_navbar(const char* title, const char* username);
void ui_print_chat_header(const char* title);
void ui_print_chat_prompt(const char* username);
void ui_clear_prompt_line(void);

#endif // UI_H
