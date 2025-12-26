#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

// =================================================================================
// FRONTEND ENGINEER NOTES (CONSOLE UI V4):
// - Professional "GUI-like" Console Experience.
// - Two-Level Menus: Welcome Screen vs Main Dashboard.
// - Seamless Chat: Dynamic Redraw with History Buffer (Zalo Style).
// - System Calls: Uses system("clear") (Linux) / system("cls") (Windows).
// =================================================================================

// UI Helpers
void clear_screen() {
    #ifdef _WIN32
    system("cls");
    #else
    system("clear");
    #endif
}

// Global State for UI
char current_username[MAX_USERNAME] = "";
char current_chat_partner[MAX_USERNAME] = "";
#define MAX_HISTORY_DISPLAY 20
char chat_history[MAX_HISTORY_DISPLAY][MAX_CONTENT];
int history_count = 0;

void add_to_history(const char* msg) {
    if (history_count < MAX_HISTORY_DISPLAY) {
        strncpy(chat_history[history_count++], msg, MAX_CONTENT-1);
    } else {
        // Shift left
        for(int i=0; i<MAX_HISTORY_DISPLAY-1; i++) strcpy(chat_history[i], chat_history[i+1]);
        strncpy(chat_history[MAX_HISTORY_DISPLAY-1], msg, MAX_CONTENT-1);
    }
}

// --- RENDER FUNCTIONS ---

void render_welcome_menu() {
    clear_screen();
    printf("==========================================\n");
    printf("         CHAT APPLICATION (v2.0)          \n");
    printf("==========================================\n");
    printf("   1. Login                               \n");
    printf("   2. Register                            \n");
    printf("   0. Exit                                \n");
    printf("==========================================\n");
    printf("Option: ");
    fflush(stdout);
}

void render_main_menu() {
    clear_screen();
    printf("==========================================\n");
    printf("     HELLO: %s | ONLINE                   \n", current_username);
    printf("==========================================\n");
    printf("--- FRIENDS MANAGEMENT ---   --- MESSAGING ---\n");
    printf(" 1. Friends List              7. CHAT (Seamless)\n");
    printf(" 2. Add Friend                8. Group Chat   \n");
    printf(" 3. Friend Requests           9. Offline Msgs \n");
    printf(" 4. Block/Unblock            10. Search Msgs  \n");
    printf(" 5. Accept Friend            12. Group Broadcast\n");
    printf(" 6. Remove Friend                             \n");
    printf("\n");
    printf("--- SYSTEM ---\n");
    printf(" 0. Logout\n");
    printf("==========================================\n");
    printf("Your choice: ");
    fflush(stdout);
}

void render_chat_screen() {
    clear_screen();
    printf("--------------------------------------------------\n");
    printf(" Chatting with: %s\n", current_chat_partner);
    printf(" (Type message and Enter. Type '/exit' to back)\n");
    printf("--------------------------------------------------\n");
    for(int i=0; i<history_count; i++) {
        printf("%s\n", chat_history[i]);
    }
    printf("--------------------------------------------------\n");
    printf("[Me]: ");
    fflush(stdout);
}

// State constants
typedef enum {
    STATE_WELCOME,
    STATE_MAIN_MENU,
    STATE_CHAT_MODE,
    // Input Sub-states
    STATE_INPUT_REG_USER, STATE_INPUT_REG_PASS,
    STATE_INPUT_LOGIN_USER, STATE_INPUT_LOGIN_PASS,
    STATE_INPUT_CHAT_TARGET,
    STATE_INPUT_ADD_FRIEND, STATE_INPUT_GROUP_NAME, STATE_INPUT_BLOCK,
    STATE_INPUT_SEARCH_KEYWORD,
    // ... others can be added as needed
} ClientState;

int interaction_step = STATE_WELCOME;
char temp_data[MAX_CONTENT]; 

int main() {
    socket_t client_socket;
    struct sockaddr_in server_addr;
    char ip[20];

    // Connection Setup (Simplified for UX)
    clear_screen();
    printf("Server IP [127.0.0.1]: ");
    if (fgets(ip, sizeof(ip), stdin)) {
        trim_newline(ip);
        if (strlen(ip) == 0) strcpy(ip, "127.0.0.1");
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) { perror("socket"); return 1; }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid Address\n"); return 1;
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed"); return 1;
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO; fds[0].events = POLLIN;
    fds[1].fd = client_socket; fds[1].events = POLLIN;

    int logged_in = 0;
    render_welcome_menu();

    CommandType pending_cmd = CMD_ERROR;

    while (1) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) break;

        // --- SERVER MESSAGES ---
        if (fds[1].revents & POLLIN) {
            char buffer[BUFFER_SIZE];
            int len = recv(client_socket, buffer, BUFFER_SIZE-1, 0);
            if (len <= 0) { printf("\nDisconnected.\n"); break; }
            buffer[len] = '\0';
            
            ProtocolMessage* msg = deserialize_protocol_message(buffer, len);
            if (msg) {
                if (msg->cmd == CMD_SUCCESS) {
                    if (pending_cmd == CMD_LOGIN) {
                        logged_in = 1;
                        interaction_step = STATE_MAIN_MENU;
                        render_main_menu();
                    } else if (pending_cmd == CMD_LOGOUT) {
                        logged_in = 0;
                        memset(current_username, 0, sizeof(current_username));
                        interaction_step = STATE_WELCOME;
                        render_welcome_menu();
                    } else {
                        // Generic Success (Friend Added, etc)
                         if (interaction_step != STATE_CHAT_MODE) {
                             printf("\n[SUCCESS] %s\n", msg->content);
                             printf("Press Enter to continue..."); fflush(stdout);
                             // We need to wait for user ACK or just redraw? 
                             // Proper UI waits. But poll loop is fast. 
                             // Let's just print and let user see it before next input redraws.
                         }
                    }
                    pending_cmd = CMD_ERROR;
                } 
                else if (msg->cmd == CMD_ERROR) {
                    printf("\n[ERROR] %s\n", msg->content);
                    if (interaction_step == STATE_INPUT_LOGIN_PASS) {
                         // Failed login, retry or back?
                         printf("Press Enter to return..."); fflush(stdout);
                         interaction_step = STATE_WELCOME; // Back to start on fail
                    }
                } 
                else if (msg->cmd == CMD_RECEIVE_MESSAGE) {
                    if (interaction_step == STATE_CHAT_MODE) {
                        char display_line[BUFFER_SIZE + 200]; // Increased to handle full input buffer
                        if (strncmp(msg->content, "[History", 8) == 0) {
                             snprintf(display_line, sizeof(display_line), "%s", msg->content);
                        } else {
                             snprintf(display_line, sizeof(display_line), "[%s]: %s", msg->sender, msg->content);
                        }
                        add_to_history(display_line);
                        render_chat_screen();
                    } else {
                        // Toast Notification
                        printf("\n[NEW MSG] %s: %s\n", msg->sender, msg->content);
                        if (interaction_step == STATE_MAIN_MENU) {
                             printf("Your choice: "); fflush(stdout); 
                        }
                    }
                }
                else {
                    // Info Messages (Lists, etc)
                    if (interaction_step != STATE_CHAT_MODE) {
                        printf("\n%s\n", msg->content);
                        if (interaction_step == STATE_MAIN_MENU) {
                             printf("Your choice: "); fflush(stdout);
                        }
                    }
                }
                free(msg);
            }
        }

        // --- USER INPUT ---
        if (fds[0].revents & POLLIN) {
            char line[BUFFER_SIZE];
            if (!fgets(line, sizeof(line), stdin)) break;
            trim_newline(line);
            
            // IGNORE EMPTY ENTER IF NOT NEEDED
            if (strlen(line) == 0 && interaction_step != STATE_CHAT_MODE) {
                 // Refresh current screen if just Enter?
                 if (interaction_step == STATE_MAIN_MENU) render_main_menu();
                 if (interaction_step == STATE_WELCOME) render_welcome_menu();
                 continue;
            }

            if (interaction_step == STATE_WELCOME) {
                if (strcmp(line, "1") == 0) { // Login
                    printf("Username: "); fflush(stdout);
                    interaction_step = STATE_INPUT_LOGIN_USER;
                } else if (strcmp(line, "2") == 0) { // Register
                    printf("New Username: "); fflush(stdout);
                    interaction_step = STATE_INPUT_REG_USER;
                } else if (strcmp(line, "0") == 0) {
                    break;
                } else {
                    printf("\nInvalid option! Please try again.\nOption: "); fflush(stdout);
                }
            }
            else if (interaction_step == STATE_INPUT_LOGIN_USER) {
                strcpy(temp_data, line);
                printf("Password: "); fflush(stdout);
                interaction_step = STATE_INPUT_LOGIN_PASS;
            }
            else if (interaction_step == STATE_INPUT_LOGIN_PASS) {
                strcpy(current_username, temp_data); // Speculatively set name
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                msg.cmd = CMD_LOGIN; strcpy(msg.sender, temp_data); strcpy(msg.content, line);
                pending_cmd = CMD_LOGIN;
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
            }
            else if (interaction_step == STATE_MAIN_MENU) {
                int choice = atoi(line);
                if (strcmp(line, "0") == 0) { // Logout
                     ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_LOGOUT; pending_cmd = CMD_LOGOUT;
                     int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                } else if (choice == 1) { // Friends List
                     ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_GET_FRIENDS;
                     int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                } else if (choice == 7) { // Chat
                     printf("Enter username to chat: "); fflush(stdout);
                     interaction_step = STATE_INPUT_CHAT_TARGET;
                } else if (choice == 2) { // Add Friend
                     printf("Username to add: "); fflush(stdout);
                     interaction_step = STATE_INPUT_ADD_FRIEND;
                } // ... Add cases for 3,4,5,6,8,9,10,12 as needed
                 else if (choice == 3) { // Requests
                     ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_GET_REQUESTS;
                     int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                 }
                 else if (choice == 4) { // Block/Unblock
                     printf("Username to block/unblock: "); fflush(stdout);
                     interaction_step = STATE_INPUT_BLOCK;
                 }
                 else if (choice == 5) { // Accept Friend
                     printf("Username to accept: "); fflush(stdout);
                     // Reuse/Add STATE_INPUT_ACCEPT_FRIEND? Let's use generic logic or ID.
                     // The enum doesn't have ACCEPT_FRIEND. Let's add it or use a generic one?
                     // Let's add STATE_INPUT_ACCEPT_FRIEND to enum or just use magic number for now to save complexity?
                     // Use magic number 300 from previous versions to be safe, or just add logic.
                     // Actually, let's map it to a new state ID 105 for clean code.
                     interaction_step = 105; 
                 }
                 else if (choice == 6) { // Remove Friend
                     printf("Username to remove: "); fflush(stdout);
                     interaction_step = 106;
                 }
                 else if (choice == 8) { // Group Chat (Create)
                     printf("Enter New Group Name: "); fflush(stdout);
                     interaction_step = STATE_INPUT_GROUP_NAME;
                 }
                 else if (choice == 9) { // 9. Unread Summary
                     ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_GET_UNREAD_SUMMARY;
                     int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                     // Server will return CMD_SUCCESS with the report string
                 }
                 else if (choice == 12) { // Group Broadcast (Group Msg)
                     printf("Enter Group ID: "); fflush(stdout);
                     interaction_step = 200; // STATE_INPUT_GROUP_MSG_ID
                 }
                 else if (choice == 10) { // Search Msgs
                     printf("Enter keyword: "); fflush(stdout);
                     interaction_step = STATE_INPUT_SEARCH_KEYWORD; // Need to ensure it's in enum or use constant
                 } 
                 else {
                     printf("\nInvalid choice! Please try again.\nYour choice: "); fflush(stdout);
                 }
            }
            else if (interaction_step == STATE_INPUT_CHAT_TARGET) {
                strcpy(current_chat_partner, line);
                history_count = 0; // Clear local history
                
                // Fetch History
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                msg.cmd = CMD_HISTORY; strcpy(msg.recipient, current_chat_partner);
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                
                interaction_step = STATE_CHAT_MODE;
                render_chat_screen();
            }
            else if (interaction_step == STATE_CHAT_MODE) {
                if (strcmp(line, "/exit") == 0) {
                    interaction_step = STATE_MAIN_MENU;
                    render_main_menu();
                } else {
                    // Send
                    ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                    msg.cmd = CMD_SEND_MESSAGE; strcpy(msg.recipient, current_chat_partner); strcpy(msg.content, line);
                    int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                    
                    // Local Echo
                    char display_line[BUFFER_SIZE + 200];
                    snprintf(display_line, sizeof(display_line), "[Me]: %s", line);
                    add_to_history(display_line);
                    render_chat_screen();
                }
            }
             else if (interaction_step == STATE_INPUT_ADD_FRIEND) {
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                msg.cmd = CMD_FRIEND_REQUEST; strcpy(msg.recipient, line);
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                printf("Request sent. Press Enter."); interaction_step = STATE_MAIN_MENU;
            }
            
            // ... registration inputs (omitted for brevity, similar to login)
            else if (interaction_step == STATE_INPUT_REG_USER) {
                 strcpy(temp_data, line); printf("Password: "); fflush(stdout); interaction_step = STATE_INPUT_REG_PASS;
            }
            else if (interaction_step == STATE_INPUT_REG_PASS) {
                 ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                 msg.cmd = CMD_REGISTER; strcpy(msg.sender, temp_data); strcpy(msg.content, line);
                 int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                 interaction_step = STATE_WELCOME; // Back to welcome after reg attempt
            }
            // --- NEW INPUT HANDLERS ---
            else if (interaction_step == STATE_INPUT_BLOCK) {
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_BLOCK_USER; strcpy(msg.recipient, line);
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                printf("Block command sent.\n"); interaction_step = STATE_MAIN_MENU; render_main_menu();
            }
            else if (interaction_step == 105) { // Accept Friend
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_FRIEND_ACCEPT; strcpy(msg.recipient, line);
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                printf("Accept sent.\n"); interaction_step = STATE_MAIN_MENU; render_main_menu();
            }
            else if (interaction_step == 106) { // Remove Friend
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_REMOVE_FRIEND; strcpy(msg.recipient, line);
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                printf("Remove sent.\n"); interaction_step = STATE_MAIN_MENU; render_main_menu();
            }
            else if (interaction_step == STATE_INPUT_GROUP_NAME) { // Create Group
                 ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_CREATE_GROUP; strcpy(msg.content, line);
                 int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                 interaction_step = STATE_MAIN_MENU; render_main_menu();
            }
            else if (interaction_step == 200) { // Group Msg ID
                strcpy(temp_data, line); // Store Group ID
                printf("Message content: "); fflush(stdout);
                interaction_step = 201;
            }
            else if (interaction_step == 201) { // Group Msg content
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                msg.cmd = CMD_GROUP_MESSAGE; strcpy(msg.recipient, temp_data); strcpy(msg.content, line);
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                interaction_step = STATE_MAIN_MENU; render_main_menu();
            }
            else if (interaction_step == STATE_INPUT_SEARCH_KEYWORD) { // 10. Search logic
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_SEARCH_HISTORY; 
                strcpy(msg.content, line); strcpy(msg.recipient, "");
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                interaction_step = STATE_MAIN_MENU; render_main_menu();
            }
        }
    }
    close(client_socket);
    return 0;
}
