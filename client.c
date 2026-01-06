#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"


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
char current_pinned_msg[MAX_CONTENT] = "";
int global_pending_friend_count = 0; // NEW: Persistent notification count

// Group Shortcut Cache
char session_groups[20][MAX_GROUP_ID];
int session_group_count = 0;

// Notification Tracking
typedef struct {
    char sender[MAX_USERNAME];
    int count;
} PendingNotif;
#define MAX_PENDING_SENDERS 20
PendingNotif pending_notifs[MAX_PENDING_SENDERS];
int pending_count = 0;

void add_pending_notification(const char* sender) {
    for(int i=0; i<pending_count; i++) {
        if(strcmp(pending_notifs[i].sender, sender) == 0) {
            pending_notifs[i].count++;
            return;
        }
    }
    if(pending_count < MAX_PENDING_SENDERS) {
        strncpy(pending_notifs[pending_count].sender, sender, MAX_USERNAME-1);
        pending_notifs[pending_count].count = 1;
        pending_count++;
    }
}

void clear_pending_notification(const char* sender) {
    for(int i=0; i<pending_count; i++) {
        if(strcmp(pending_notifs[i].sender, sender) == 0) {
            // Remove by shifting
            for(int j=i; j<pending_count-1; j++) pending_notifs[j] = pending_notifs[j+1];
            pending_count--;
            i--; // Recheck index
        }
    }
}

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
    printf(" 2. Add Friend                8. Create Group\n");
    printf(" 3. Friend Requests           9. Unread Summary\n");
    printf(" 4. Block/Unblock            10. Search Msgs\n");
    printf(" 5. Accept Friend            11. Message Group\n");
    printf(" 6. Remove Friend            12. Invite to My Group\n");
    printf("                             13. My Groups\n");
    printf("\n");
    if (global_pending_friend_count > 0) {
        // ANSI Yellow background, black text: \033[43;30m
        printf("\033[43;30m [!] NOTICE: You have %d pending friend request(s)! Check Option 3 or 5. \033[0m\n\n", 
               global_pending_friend_count);
    }
    printf("--- SYSTEM ---\n");
    printf(" 0. Logout\n");
    printf("==========================================\n");
    printf("Your choice: ");
    fflush(stdout);
}

void render_chat_screen() {
    clear_screen();
    printf("--------------------------------------------------\n");
    if (strncmp(current_chat_partner, "GRP_", 4) == 0) {
        printf(" GROUP CHAT: %s\n", current_chat_partner);
    } else {
        printf(" Chatting with: %s\n", current_chat_partner);
    }
    printf(" (Type message and Enter. Type '/exit' to back)\n");
    // Show Pending Notifications in Blue
    for(int i=0; i<pending_count; i++) {
         // ANSI Blue: \033[1;34m, Reset: \033[0m
         printf("\033[1;34m [!] You have %d pending message(s) from %s\033[0m\n", 
                pending_notifs[i].count, pending_notifs[i].sender);
    }
    if (strlen(current_pinned_msg) > 0) {
        printf("\033[1;34m PINNED: %s\033[0m\n", current_pinned_msg);
        printf("--------------------------------------------------\n");
    }
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
    STATE_INPUT_ADD_FRIEND, STATE_INPUT_GROUP_NAME,
    STATE_INPUT_SEARCH_KEYWORD,
    STATE_WAIT_ENTER,
    // ... others can be added as needed
} ClientState;

int interaction_step = STATE_WELCOME;
char temp_data[MAX_CONTENT]; 

int main(int argc, char *argv[]) {
    socket_t client_socket;
    struct sockaddr_in server_addr;
    char ip[50];

    // Connection Setup
    // Priority 1: Command Line Argument
    if (argc > 1) {
        strncpy(ip, argv[1], sizeof(ip)-1);
        ip[sizeof(ip)-1] = '\0';
        printf("Connecting to Server IP from argument: %s\n", ip);
    } 
    // Priority 2: Interactive Prompt
    else {
        clear_screen();
        printf("Server IP [127.0.0.1]: ");
        if (fgets(ip, sizeof(ip), stdin)) {
            trim_newline(ip);
            if (strlen(ip) == 0) strcpy(ip, "127.0.0.1");
        } else {
             strcpy(ip, "127.0.0.1");
        }
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
                        interaction_step = STATE_MAIN_MENU;
                        render_main_menu();
                    } else if (pending_cmd == CMD_LOGOUT) {
                        memset(current_username, 0, sizeof(current_username));
                        interaction_step = STATE_WELCOME;
                        render_welcome_menu();
                    } else {
                        // Generic Success (Friend Added, etc)
                         if (interaction_step != STATE_CHAT_MODE) {
                              bool owned_list = (strncmp(msg->content, "Owned Groups:", 12) == 0);
                              bool join_list = (strncmp(msg->content, "Your Groups:", 12) == 0);

                              if (owned_list || join_list) {
                                   // PARSE GROUP IDs for Shortcut
                                   session_group_count = 0;
                                   char* tmp = strdup(msg->content);
                                   char* l_ptr = strtok(tmp, "\n");
                                   while (l_ptr && session_group_count < 20) {
                                       char* bracket = strchr(l_ptr, '[');
                                       char* close_bracket = strchr(l_ptr, ']');
                                       if (bracket && close_bracket && close_bracket > bracket) {
                                           int len = close_bracket - bracket - 1;
                                           if (len < MAX_GROUP_ID) {
                                               strncpy(session_groups[session_group_count], bracket + 1, len);
                                               session_groups[session_group_count][len] = '\0';
                                               session_group_count++;
                                           }
                                       }
                                       l_ptr = strtok(NULL, "\n");
                                   }
                                   free(tmp);
                                   printf("\n%s", msg->content); fflush(stdout);
                                   if (session_group_count > 0) {
                                       interaction_step = owned_list ? 1201 : 300;
                                   } else {
                                       printf("\nPress Enter to continue..."); fflush(stdout);
                                       interaction_step = STATE_WAIT_ENTER;
                                   }
                              } else if (pending_cmd == CMD_GET_REQUESTS && interaction_step == 104) {
                                   printf("\n[SUCCESS] %s\n", msg->content);
                                   printf("\n--- SELECT USER FROM LIST ABOVE ---\n");
                                   printf("Enter Username to handle: "); fflush(stdout);
                              } else {
                                   printf("\n[SUCCESS] %s\n", msg->content);
                                   printf("Press Enter to continue..."); fflush(stdout);
                                   interaction_step = STATE_WAIT_ENTER;
                              }
                             }
                         }
                    pending_cmd = CMD_ERROR;
                } 
                else if (msg->cmd == CMD_ERROR) {
                    printf("\n[ERROR] %s\n", msg->content);
                    printf("Press Enter to continue..."); fflush(stdout);
                    interaction_step = STATE_WAIT_ENTER;
                    if (pending_cmd == CMD_LOGIN) {
                         interaction_step = STATE_WELCOME; // Special case: back to welcome on login fail
                    }
                } 
                else if (msg->cmd == CMD_RECEIVE_MESSAGE) {
                    // Logic:
                    // 1. If we are in CHAT_MODE with "PartnerX", and msg is from "PartnerX", show it.
                    // 2. If msg is from someone else, notifying "New message from Y" (but don't clutter chat screen).
                    // 3. For Groups: current_chat_partner starts with "GRP_". Msg sender is usually a user, but content has "[Group Name]" tag? 
                    //    Wait, server sends Group Msg as: sender=ActualUser, content="[Group G] Content".
                    //    But client needs to filter based on GroupID? 
                    //    Actually server sends CMD_RECEIVE_MESSAGE. Who is the "channel"? 
                    //    In current server code, Group messages are sent to members.
                    //    The client needs to know if this message belongs to the current "Room".
                    //    Simplification: Server logic for groups puts "[Group Name]" in content. 
                    //    Let's rely on standard sender check for Private, and Tag check for Group.
                    
                    bool show_in_chat = false;
                    
                    if (interaction_step == STATE_CHAT_MODE) {
                        // Case 1: Private Chat (Partner is User)
                        if (strncmp(current_chat_partner, "GRP_", 4) != 0) { // We are in Private Chat
                             // Show message IF:
                             // 1. Sender matches Partner
                             // 2. AND Content does NOT look like a Group Message (`[Group ...`)
                             if (strcmp(msg->sender, current_chat_partner) == 0 && strncmp(msg->content, "[Group ", 7) != 0) {
                                 if (strncmp(msg->sender, "PINNED_SYSTEM", 13) == 0) {
                                  strncpy(current_pinned_msg, msg->content, MAX_CONTENT-1);
                                  render_chat_screen();
                              } else {
                                  show_in_chat = true;
                              }
                             }
                        }
                        // Case 2: Group Chat (Partner is Group ID)
                        else { // We are in Group Chat
                             // Show message IF:
                             // 1. Content looks like a Group Message
                             // 2. (Optional) Ideally we check if `recipient` matches `current_chat_partner`, but server sends Recipient=GroupID.
                             //    However, `msg` struct in client might not preserve it correctly if deserializer is weird?
                             //    Let's trust `strncmp(msg->content, "[Group ", 7) == 0`.
                             //    Wait, if I am in Group A, and get message for Group B?
                             //    I should check if `msg->recipient` == `current_chat_partner`.
                             //    Let's assume `msg->recipient` is correctly populated by `deserialize_protocol_message`.
                             
                             if (strncmp(msg->content, "[Group ", 7) == 0) {
                                  // Verify it matches THIS group if possible. 
                                  // Server sends CMD_GROUP_MESSAGE -> sends CMD_RECEIVE_MESSAGE to members.
                                  // The `fwd` message has `recipient` set to GroupID.
                                  if (strcmp(msg->recipient, current_chat_partner) == 0) {
                                      show_in_chat = true;
                                  }
                             }
                        }
                        
                        // Case 3: Self-sent (Echo), Search, History
                        if (strcmp(msg->sender, current_username) == 0 || 
                                 strncmp(msg->sender, "SEARCH", 6) == 0 ||
                                 strncmp(msg->sender, "HISTORY", 7) == 0 ||
                                 strncmp(msg->sender, "SYSTEM", 6) == 0 || strncmp(msg->sender, "PINNED_SYSTEM", 13) == 0) {
                             show_in_chat = true;
                        }
                    }

                    if (show_in_chat) {
                        char display_line[BUFFER_SIZE + 200]; 
                        // If it's a History/Search message, the content usually already has the format [Sender Time] Content
                        // But wait, our history format is "[Sender Date] Content".
                        // Standard chat format is "[Sender]: Content".
                        // Logic: If sender is HISTORY or SEARCH, print content as-is.
                        if (strcmp(msg->sender, "HISTORY") == 0 || strcmp(msg->sender, "SEARCH") == 0) {
                            snprintf(display_line, sizeof(display_line), "%s", msg->content);
                        } 
                        else if (strncmp(msg->content, "[History", 8) == 0) { // Legacy check
                             snprintf(display_line, sizeof(display_line), "%s", msg->content);
                        } else {
                             snprintf(display_line, sizeof(display_line), "[%s]: %s", msg->sender, msg->content);
                        }
                        add_to_history(display_line);
                        render_chat_screen();
                    } else {
                        // Notification for background message
                        if (interaction_step == STATE_CHAT_MODE) {
                            // Filter system/search messages
                            if (strcmp(msg->sender, "SYSTEM") == 0 || strcmp(msg->sender, "SEARCH") == 0) {
                                // Ignore
                            } else {
                                // Valid notification
                                // IMPROVEMENT: If it's a group message, show notification from "Group X" instead of "User Y"
                                char noti_source[MAX_USERNAME];
                                strncpy(noti_source, msg->sender, MAX_USERNAME-1);
                                
                                if (strncmp(msg->content, "[Group ", 7) == 0) {
                                    // Extract Group Name: "[Group Name] Content"
                                    char* start = msg->content + 7;
                                    char* end = strstr(start, "]");
                                    if (end) {
                                        int len = end - start;
                                        if (len > 0 && len < 50) {
                                            char group_name[51];
                                            strncpy(group_name, start, len);
                                            group_name[len] = '\0';
                                            snprintf(noti_source, sizeof(noti_source), "Group %s", group_name);
                                        }
                                    }
                                }
                                
                                add_pending_notification(noti_source);
                                render_chat_screen(); // Redraw
                                printf("\a"); // Beep
                            }
                        } else {
                            printf("\n[NEW MSG] %s: %s\n", msg->sender, msg->content);
                            if (interaction_step == STATE_MAIN_MENU) {
                                 printf("Your choice: "); fflush(stdout); 
                            }
                        }
                    }
                }
                else {
                    // Info Messages (Lists, etc)
                    if (interaction_step != STATE_CHAT_MODE) {
                        // NEW: Check for Friend Count update
                        if (strncmp(msg->content, "[FRIEND_COUNT] ", 15) == 0) {
                            global_pending_friend_count = atoi(msg->content + 15);
                            if (interaction_step == STATE_MAIN_MENU) render_main_menu();
                        } else {
                            printf("\n%s\n", msg->content);
                            if (interaction_step == STATE_MAIN_MENU) {
                                 printf("Your choice: "); fflush(stdout);
                            }
                        }
                    } else {
                        // If in chat mode, we might still want to catch friend request updates in the background
                         if (strncmp(msg->content, "[FRIEND_COUNT] ", 15) == 0) {
                            global_pending_friend_count = atoi(msg->content + 15);
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
            if (strlen(line) == 0 && interaction_step != STATE_CHAT_MODE && interaction_step != STATE_WAIT_ENTER && interaction_step != 300 && interaction_step != 1201) {
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
                     printf("Enter username or Group ID to chat: "); fflush(stdout);
                     interaction_step = STATE_INPUT_CHAT_TARGET;
                } else if (choice == 2) { // Add Friend
                     printf("Username to add: "); fflush(stdout);
                     interaction_step = STATE_INPUT_ADD_FRIEND;
                } // ... Add cases for 3,4,5,6,8,9,10,12 as needed
                 else if (choice == 3) { // Requests
                     ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_GET_REQUESTS;
                     int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                      pending_cmd = CMD_GET_REQUESTS;
                 }
                 else if (choice == 4) { // Block/Unblock
                     printf("Enter Username to block/unblock: "); fflush(stdout);
                     interaction_step = 400;
                 }
                 else if (choice == 5) { // Accept/Reject Friend (Enhanced)
                      ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_GET_REQUESTS;
                      int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                       pending_cmd = CMD_GET_REQUESTS;
                      interaction_step = 104; 
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
                      pending_cmd = CMD_GET_UNREAD_SUMMARY;
                     // Server will return CMD_SUCCESS with the report string
                 }
                 else if (choice == 11) { // 11. Message Group
                     printf("Enter Group ID: "); fflush(stdout);
                     interaction_step = 200; // STATE_INPUT_GROUP_MSG_ID
                 }
                  else if (choice == 12) { // 12. Invite to My Group
                      ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_LIST_OWNED_GROUPS;
                      int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                      pending_cmd = CMD_LIST_OWNED_GROUPS;
                  }
                  else if (choice == 13) { // 13. My Groups
                      ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_LIST_GROUPS;
                      int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                  }
                 else if (choice == 10) { // Search Msgs
                     printf("Enter keyword: "); fflush(stdout);
                     interaction_step = STATE_INPUT_SEARCH_KEYWORD; 
                 } 
                 else {
                     printf("\nInvalid choice! Please try again.\nYour choice: "); fflush(stdout);
                 }
            }
            else if (interaction_step == STATE_INPUT_CHAT_TARGET) {
                if (strlen(line) > 0) {
                     strncpy(current_chat_partner, line, MAX_USERNAME-1);
                     clear_pending_notification(line); // Clear notifications
                     history_count = 0; // Clear local history
                     
                     // Fetch History
                     ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                     msg.cmd = CMD_HISTORY; strcpy(msg.recipient, current_chat_partner);
                     int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                     
                     interaction_step = STATE_CHAT_MODE;
                     memset(current_pinned_msg, 0, MAX_CONTENT); // Clear old pin before loading new one
                     render_chat_screen();
                } else {
                     printf("Invalid username. Enter again: "); fflush(stdout);
                }
            }
            else if (interaction_step == STATE_CHAT_MODE) {
                if (strcmp(line, "/exit") == 0) {
                    ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_EXIT_CHAT;
                    int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                    interaction_step = STATE_MAIN_MENU;
                    memset(current_pinned_msg, 0, MAX_CONTENT);
                    render_main_menu();
                } else if (strncmp(line, "/pin ", 5) == 0) {
                    ProtocolMessage msg; memset(&msg, 0, sizeof(msg));
                    msg.cmd = CMD_PIN_MESSAGE;
                    strncpy(msg.content, line + 5, MAX_CONTENT - 1);
                    int l; char* b = serialize_protocol_message(&msg, &l);
                    send_all(client_socket, b, l); free(b);
                } else {
                    // Send
                    ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                    
                    if (strncmp(current_chat_partner, "GRP_", 4) == 0) {
                        msg.cmd = CMD_GROUP_MESSAGE;
                    } else {
                        msg.cmd = CMD_SEND_MESSAGE;
                    }
                    strcpy(msg.recipient, current_chat_partner); strcpy(msg.content, line);
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
                printf("Processing request..."); fflush(stdout); 
            }
            
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
            else if (interaction_step == 400) { // Block/Unblock Username
                if (strlen(line) > 0) {
                    strncpy(temp_data, line, MAX_USERNAME - 1);
                    printf("Action for %s? (B)lock / (U)nblock / (C)ancel: ", temp_data); fflush(stdout);
                    interaction_step = 401;
                } else {
                    interaction_step = STATE_MAIN_MENU; render_main_menu();
                }
            }
            else if (interaction_step == 401) { // Block/Unblock Action
                if (line[0] == 'b' || line[0] == 'B') {
                    ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_BLOCK_USER; strcpy(msg.recipient, temp_data);
                    int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                    printf("Sending Block request...\n");
                } else if (line[0] == 'u' || line[0] == 'U') {
                    ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_UNBLOCK_USER; strcpy(msg.recipient, temp_data);
                    int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                    printf("Sending Unblock request...\n");
                }
                interaction_step = STATE_MAIN_MENU; render_main_menu();
            }
            else if (interaction_step == 104) { // 104. Select User from Request List
                if (strlen(line) > 0) {
                    strncpy(temp_data, line, MAX_USERNAME - 1);
                    printf("Action for %s? (A)ccept / (R)eject / (C)ancel: ", temp_data); fflush(stdout);
                    interaction_step = 105;
                } else {
                    interaction_step = STATE_MAIN_MENU; render_main_menu();
                }
            }
            else if (interaction_step == 105) { // 105. Decide Accept/Reject
                if (line[0] == 'a' || line[0] == 'A') {
                    ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_FRIEND_ACCEPT; strcpy(msg.recipient, temp_data);
                    int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                    printf("Processing Accept...\n");
                } else if (line[0] == 'r' || line[0] == 'R') {
                    ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_FRIEND_REJECT; strcpy(msg.recipient, temp_data);
                    int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                    printf("Processing Reject...\n");
                }
                interaction_step = STATE_WAIT_ENTER;
                printf("\nPress Enter to return to Menu."); fflush(stdout);
            }
            else if (interaction_step == STATE_WAIT_ENTER) {
                interaction_step = STATE_MAIN_MENU;
                render_main_menu();
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
            else if (interaction_step == 1201) { // Select Owned Group for Invitation
                if (strlen(line) == 0) {
                    interaction_step = STATE_MAIN_MENU; render_main_menu();
                } else {
                    int idx = atoi(line);
                    if (idx >= 1 && idx <= session_group_count) {
                        strncpy(temp_data, session_groups[idx-1], MAX_CONTENT-1);
                        printf("Enter Username to invite to %s: ", temp_data); fflush(stdout);
                        interaction_step = 1202;
                    } else {
                        printf("Invalid index! Enter 1-%d or press Enter to cancel: ", session_group_count); fflush(stdout);
                    }
                }
            }
            else if (interaction_step == 1202) { // Enter Username to invite
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); msg.cmd = CMD_ADD_TO_GROUP; 
                snprintf(msg.content, MAX_CONTENT, "%.500s %.500s", temp_data, line); // temp_data=GID, line=User
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                printf("Processing Invitation...\n");
                // CMD_SUCCESS will handle the rest
            }
            else if (interaction_step == 200) { // Group Msg ID (Legacy/Direct)
                strcpy(temp_data, line); // Store Group ID
                printf("Message content: "); fflush(stdout);
                interaction_step = 201;
            }
            else if (interaction_step == 201) { // Group Msg content
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                msg.cmd = CMD_GROUP_MESSAGE; strcpy(msg.recipient, temp_data); strcpy(msg.content, line);
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                printf("Group message sent. Press Enter."); fflush(stdout); interaction_step = STATE_WAIT_ENTER; fflush(stdout);
            }
            else if (interaction_step == 202) { // Group Invite ID
                strcpy(temp_data, line);
                printf("Enter Username to invite: "); fflush(stdout);
                interaction_step = 203;
            }
            else if (interaction_step == 203) { // Group Invite User
                ProtocolMessage msg; memset(&msg,0,sizeof(msg)); 
                msg.cmd = CMD_ADD_TO_GROUP; 
                snprintf(msg.content, MAX_CONTENT, "%.500s %.500s", temp_data, line); // temp_data=GID, line=User
                int l; char* b = serialize_protocol_message(&msg, &l); send_all(client_socket,b,l); free(b);
                printf("Inviting... Press Enter."); fflush(stdout); interaction_step = STATE_WAIT_ENTER; fflush(stdout);
            }
            else if (interaction_step == 300) { // Group Shortcut Selection
                if (strlen(line) == 0) {
                    interaction_step = STATE_MAIN_MENU;
                    render_main_menu();
                } else {
                    int idx = atoi(line);
                    if (idx >= 1 && idx <= session_group_count) {
                        strncpy(current_chat_partner, session_groups[idx-1], MAX_USERNAME-1);
                        ProtocolMessage req; memset(&req,0,sizeof(req));
                        req.cmd = CMD_HISTORY; strncpy(req.recipient, current_chat_partner, MAX_USERNAME-1);
                        int l; char* b = serialize_protocol_message(&req, &l); send_all(client_socket,b,l); free(b);
                        
                        interaction_step = STATE_CHAT_MODE;
                        history_count = 0;
                        clear_screen();
                        printf("Entering Chat with %s...\n", current_chat_partner);
                    } else {
                        printf("Invalid index! Enter 1-%d or press Enter to cancel: ", session_group_count);
                        fflush(stdout);
                    }
                }
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
