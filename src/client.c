#include "../include/client.h"
#include "../include/ui.h"
#include "../include/common.h"
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

socket_t client_socket;
bool is_connected = false;
char current_username[MAX_USERNAME] = "";

// UI State Management
typedef enum {
    UI_STATE_MENU = 0,      // Dashboard/Menu view
    UI_STATE_CHAT_1V1 = 1,  // 1-1 Chat room
    UI_STATE_CHAT_GROUP = 2 // Group chat room
} UIState;

static UIState current_ui_state = UI_STATE_MENU;
static char current_chat_recipient[MAX_USERNAME] = "";  // For 1-1 chat
static char current_chat_group[MAX_GROUP_ID] = "";      // For group chat

// Initialize client socket
int init_client(socket_t* client_socket, const char* server_ip) {
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }
    #endif

    *client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*client_socket == INVALID_SOCKET) {
        #ifdef _WIN32
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        #else
        printf("Socket creation failed: %s\n", strerror(errno));
        #endif
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    #ifdef _WIN32
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        printf("Invalid address: %s\n", server_ip);
        close_socket(*client_socket);
        WSACleanup();
        return -1;
    }
    #else
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid address: %s\n", server_ip);
        close_socket(*client_socket);
        return -1;
    }
    #endif

    if (connect(*client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        #ifdef _WIN32
        printf("Connection failed: %d\n", WSAGetLastError());
        #else
        printf("Connection failed: %s\n", strerror(errno));
        #endif
        close_socket(*client_socket);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return -1;
    }

    is_connected = true;
    printf("Connected to server\n");
    return 0;
}

// Send command to server
void send_command(socket_t socket, ProtocolMessage* msg) {
    int len;
    char* buffer = serialize_protocol_message(msg, &len);
    if (buffer) {
        int sent = send_all(socket, buffer, len);
        if (sent == -1) {
            #ifdef _WIN32
            printf("Send failed: %d\n", WSAGetLastError());
            #else
            printf("Send failed: %s\n", strerror(errno));
            #endif
            is_connected = false;
        }
        free(buffer);
    }
}

// Receive response from server
void receive_response(socket_t socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        is_connected = false;
        return;
    }
    
    buffer[bytes_received] = '\0';
    ProtocolMessage* msg = deserialize_protocol_message(buffer, bytes_received);
    if (!msg) return;
    
    switch (msg->cmd) {
        case CMD_RECEIVE_MESSAGE: {
            // Sound notification
            printf("\a");
            fflush(stdout);
            
            // Check if this is a notification (system message or empty sender)
            bool is_notification = (msg->msg_type == MSG_SYSTEM) || 
                                  (strlen(msg->sender) == 0 || msg->sender[0] == '\0');
            
            time_t now = time(NULL);
            struct tm* timeinfo = localtime(&now);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
            
            // Handle based on current UI state
            if (current_ui_state == UI_STATE_MENU) {
                // In menu mode: only show brief notification
                if (is_notification) {
                    printf("\n%s[%s]%s %s%s%s\n", COLOR_SYSTEM, time_str, COLOR_RESET, COLOR_SYSTEM, msg->content, COLOR_RESET);
                } else {
                    // Brief notification for incoming message
                    printf("\n%s[!]%s New message from %s%s%s. %sGo to chat to view.%s\n", 
                           COLOR_WARNING, COLOR_RESET, COLOR_INFO, msg->sender, COLOR_RESET, 
                           COLOR_SYSTEM, COLOR_RESET);
                }
            } else {
                // In chat room mode: show full message with bubble
                if (is_notification) {
                    printf("\n%s[%s]%s %s%s%s\n", COLOR_SYSTEM, time_str, COLOR_RESET, COLOR_SYSTEM, msg->content, COLOR_RESET);
                } else {
                    // Check if message is relevant to current chat
                    bool is_relevant = false;
                    if (current_ui_state == UI_STATE_CHAT_1V1) {
                        is_relevant = (strcmp(msg->sender, current_chat_recipient) == 0) ||
                                     (strcmp(msg->sender, current_username) == 0);
                    } else if (current_ui_state == UI_STATE_CHAT_GROUP) {
                        // For group chat, show all messages
                        is_relevant = true;
                    }
                    
                    if (is_relevant) {
                        // Determine if message is from me
                        int is_me = (strcmp(msg->sender, current_username) == 0);
                        ui_print_message_bubble_simple(msg->sender, msg->content, is_me, time_str, current_username);
                    }
                }
            }
            fflush(stdout);
            break;
        }
        case CMD_SUCCESS:
            ui_print_notification(msg->content, NOTIF_SUCCESS);
            /* Khi đang ở trong phòng chat, sau khi in notification
             * cần vẽ lại prompt ở dòng cuối để tránh bị trôi/lặp.
             */
            if (current_ui_state == UI_STATE_CHAT_1V1 || current_ui_state == UI_STATE_CHAT_GROUP) {
                ui_print_chat_prompt(current_username);
            }
            break;
        case CMD_ERROR:
            ui_print_notification(msg->content, NOTIF_ERROR);
            if (current_ui_state == UI_STATE_CHAT_1V1 || current_ui_state == UI_STATE_CHAT_GROUP) {
                ui_print_chat_prompt(current_username);
            }
            break;
        case CMD_GET_FRIENDS:
            printf("%s\n", msg->content);
            break;
        case CMD_GET_FRIEND_REQUESTS:
            printf("\n%s%s%s\n", COLOR_INFO, msg->content, COLOR_RESET);
            break;
        case CMD_SEARCH_HISTORY:
            printf("\n%s%s%s\n", COLOR_INFO, msg->content, COLOR_RESET);
            break;
        case CMD_GET_PINNED:
            printf("\n%s%s%s\n", COLOR_INFO, msg->content, COLOR_RESET);
            break;
        case CMD_GET_CHAT_HISTORY: {
            if (current_ui_state == UI_STATE_MENU) {
                printf("\n%s%s%s\n", COLOR_INFO, msg->content, COLOR_RESET);
            } else {
                if (strcmp(msg->content, "NO_HISTORY") == 0) {
                    printf("%sNo previous messages.%s\n", COLOR_SYSTEM, COLOR_RESET);
                } else {
                    char history_copy[MAX_CONTENT];
                    strncpy(history_copy, msg->content, sizeof(history_copy) - 1);
                    history_copy[sizeof(history_copy) - 1] = '\0';
                    
                    char* line_ptr = history_copy;
                    while (line_ptr && *line_ptr) {
                        char* next_line = strchr(line_ptr, '\n');
                        if (next_line) {
                            *next_line = '\0';
                        }
                        
                        if (strlen(line_ptr) > 0) {
                            char line_buffer[BUFFER_SIZE];
                            strncpy(line_buffer, line_ptr, sizeof(line_buffer) - 1);
                            line_buffer[sizeof(line_buffer) - 1] = '\0';
                            
                            char* timestamp = strtok(line_buffer, "|");
                            char* sender = strtok(NULL, "|");
                            char* body = strtok(NULL, "|");
                            
                            if (timestamp && sender && body) {
                                int is_me = (strcmp(sender, current_username) == 0);
                                ui_print_message_bubble_simple(sender, body, is_me, timestamp, NULL);
                            }
                        }
                        
                        if (!next_line) {
                            break;
                        }
                        line_ptr = next_line + 1;
                    }
                }
                
                if (current_ui_state != UI_STATE_MENU) {
                    ui_print_chat_prompt(current_username);
                }
            }
            break;
        }
        default:
            printf("Response: %s\n", msg->content);
            break;
    }
    
    free(msg);
}

static void request_chat_history(const char* target, bool is_group) {
    ProtocolMessage history_msg;
    memset(&history_msg, 0, sizeof(ProtocolMessage));
    history_msg.cmd = CMD_GET_CHAT_HISTORY;
    strncpy(history_msg.recipient, target, MAX_USERNAME - 1);
    if (is_group) {
        strncpy(history_msg.extra_data, "GROUP", sizeof(history_msg.extra_data) - 1);
    }
    send_command(client_socket, &history_msg);
}

// Receive thread function
void* receive_thread(void* arg) {
    socket_t socket = *(socket_t*)arg;
    while (is_connected) {
        receive_response(socket);
    }
    return NULL;
}

// Print menu with organized sections (numbered 1-20)
void print_menu() {
    ui_print_menu_header("Chat Application Menu");  // Don't clear screen to preserve notifications
    
    // Group 1: Account & Social (1-10)
    ui_print_section_header("Account & Social");
    ui_print_menu_item(1, "Register");
    ui_print_menu_item(2, "Login");
    ui_print_menu_item(3, "Get Friends List");
    ui_print_menu_item(4, "Add Friend (Direct)");
    ui_print_menu_item(5, "Send Friend Request");
    ui_print_menu_item(6, "Get Friend Requests");
    ui_print_menu_item(7, "Accept Friend Request");
    ui_print_menu_item(8, "Reject Friend Request");
    ui_print_menu_item(9, "Unfriend / Remove Friend");
    ui_print_menu_item(10, "Block User");
    ui_print_menu_item(22, "Unblock User");
    
    // Group 2: Chatting (11-20)
    ui_print_section_header("Chatting");
    ui_print_menu_item(11, "Send Message (1-1)");
    ui_print_menu_item(12, "Search Chat History");
    ui_print_menu_item(13, "Create Group");
    ui_print_menu_item(14, "Add User to Group");
    ui_print_menu_item(15, "Remove User from Group");
    ui_print_menu_item(16, "Leave Group");
    ui_print_menu_item(17, "Send Group Message");
    ui_print_menu_item(18, "Set Group Name / Rename Group");
    ui_print_menu_item(19, "Pin Message");
    ui_print_menu_item(20, "Get Pinned Messages");
    
    // Group 3: System (21-22)
    ui_print_section_header("System");
    ui_print_menu_item(21, "Disconnect");
    ui_print_menu_item(0, "Exit");
    
    printf("\n");
    ui_input_prompt("Choice");
}

// Handle user input
void handle_user_input(socket_t socket, const char* username) {
    char input[BUFFER_SIZE];
    int choice;
    
    while (is_connected) {
        print_menu();
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        choice = atoi(input);
        ProtocolMessage msg;
        memset(&msg, 0, sizeof(ProtocolMessage));
        strncpy(msg.sender, username, MAX_USERNAME - 1);
        
        switch (choice) {
            case 1: {  // Register
                char password[MAX_USERNAME];
                ui_input_prompt("Username");
                fgets(msg.sender, sizeof(msg.sender), stdin);
                trim_newline(msg.sender);
                ui_input_prompt("Password");
                fgets(password, sizeof(password), stdin);
                trim_newline(password);
                
                msg.cmd = CMD_REGISTER;
                strncpy(msg.content, password, MAX_CONTENT - 1);
                send_command(socket, &msg);
                break;
            }
            
            case 2: {  // Login
                char password[MAX_USERNAME];
                ui_input_prompt("Username");
                fgets(msg.sender, sizeof(msg.sender), stdin);
                trim_newline(msg.sender);
                ui_input_prompt("Password");
                fgets(password, sizeof(password), stdin);
                trim_newline(password);
                
                strncpy(current_username, msg.sender, MAX_USERNAME - 1);
                msg.cmd = CMD_LOGIN;
                strncpy(msg.content, password, MAX_CONTENT - 1);
                send_command(socket, &msg);
                break;
            }
            
            case 3: {  // Get Friends
                msg.cmd = CMD_GET_FRIENDS;
                send_command(socket, &msg);
                break;
            }
            
            case 4: {  // Add Friend (Direct - Legacy)
                ui_input_prompt("Username to add as friend");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_ADD_FRIEND;
                send_command(socket, &msg);
                break;
            }
            
            case 5: {  // Send Friend Request
                ui_input_prompt("Username to send friend request");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_SEND_FRIEND_REQUEST;
                send_command(socket, &msg);
                break;
            }
            
            case 6: {  // Get Friend Requests
                msg.cmd = CMD_GET_FRIEND_REQUESTS;
                send_command(socket, &msg);
                break;
            }
            
            case 7: {  // Accept Friend Request
                // First, show the list of friend requests
                msg.cmd = CMD_GET_FRIEND_REQUESTS;
                send_command(socket, &msg);
                
                #ifdef _WIN32
                Sleep(500);
                #else
                usleep(500000);
                #endif
                
                // Now ask for the username
                ui_input_prompt("Username to accept request from");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_ACCEPT_FRIEND_REQUEST;
                send_command(socket, &msg);
                break;
            }
            
            case 8: {  // Reject Friend Request
                // First, show the list of friend requests
                msg.cmd = CMD_GET_FRIEND_REQUESTS;
                send_command(socket, &msg);
                
                #ifdef _WIN32
                Sleep(500);
                #else
                usleep(500000);
                #endif
                
                // Now ask for the username
                ui_input_prompt("Username to reject request from");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_REJECT_FRIEND_REQUEST;
                send_command(socket, &msg);
                break;
            }
            
            case 9: {  // Unfriend / Remove Friend
                // First, show the list of friends to help user choose
                strncpy(msg.sender, current_username, MAX_USERNAME - 1);
                msg.cmd = CMD_GET_FRIENDS;
                send_command(socket, &msg);
                
                #ifdef _WIN32
                Sleep(500);
                #else
                usleep(500000);
                #endif
                
                // Now ask for the username
                ui_input_prompt("Username to unfriend");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                // Validate recipient
                if (strlen(msg.recipient) == 0) {
                    ui_print_notification("Username cannot be empty", NOTIF_ERROR);
                    clear_stdin_buffer();
                    break;
                }
                
                // Ensure sender is set for UNFRIEND command
                strncpy(msg.sender, current_username, MAX_USERNAME - 1);
                msg.cmd = CMD_UNFRIEND;
                send_command(socket, &msg);
                break;
            }
            
            case 10: {  // Block User
                ui_input_prompt("Username to block");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_BLOCK_USER;
                send_command(socket, &msg);
                break;
            }
            
            case 22: {  // Unblock User
                ui_input_prompt("Username to unblock");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_UNBLOCK_USER;
                send_command(socket, &msg);
                break;
            }
            
            case 11: {  // Chat 1-1 (Enter Chat Room)
                ui_input_prompt("Recipient username");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                // Validate recipient
                if (strlen(msg.recipient) == 0) {
                    ui_print_notification("Recipient username cannot be empty", NOTIF_ERROR);
                    clear_stdin_buffer();
                    break;
                }
                
                // Enter chat room mode
                current_ui_state = UI_STATE_CHAT_1V1;
                strncpy(current_chat_recipient, msg.recipient, MAX_USERNAME - 1);
                
                // Clear screen and show chat header
                ui_print_chat_header(msg.recipient);
                request_chat_history(msg.recipient, false);
                
                printf("%sLoading chat history...%s\n\n", COLOR_INFO, COLOR_RESET);
                
                // Enter chat input loop
                char chat_input[BUFFER_SIZE];
                printf("%sType '/back' or '/menu' to return to menu.%s\n\n", COLOR_SYSTEM, COLOR_RESET);
                ui_print_chat_prompt(current_username);
                
                while (current_ui_state == UI_STATE_CHAT_1V1 && is_connected) {
                    if (fgets(chat_input, sizeof(chat_input), stdin) == NULL) {
                        break;
                    }
                    
                    trim_newline(chat_input);
                    
                    // Check for commands
                    if (strcmp(chat_input, "/back") == 0 || strcmp(chat_input, "/menu") == 0) {
                        current_ui_state = UI_STATE_MENU;
                        current_chat_recipient[0] = '\0';
                        break;
                    }
                    
                    // Validate message
                    if (strlen(chat_input) == 0) {
                        ui_print_chat_prompt(current_username);
                        continue;
                    }
                    
                    if (strlen(chat_input) >= MAX_CONTENT - 1) {
                        printf("%sMessage too long!%s\n", COLOR_ERROR, COLOR_RESET);
                        ui_print_chat_prompt(current_username);
                        continue;
                    }
                    
                    // Send message
                    ProtocolMessage chat_msg;
                    memset(&chat_msg, 0, sizeof(ProtocolMessage));
                    strncpy(chat_msg.sender, current_username, MAX_USERNAME - 1);
                    strncpy(chat_msg.recipient, current_chat_recipient, MAX_USERNAME - 1);
                    strncpy(chat_msg.content, chat_input, MAX_CONTENT - 1);
                    chat_msg.cmd = CMD_SEND_MESSAGE;
                    chat_msg.msg_type = MSG_TEXT;
                    
                    send_command(socket, &chat_msg);
                    
                    char* ts = get_timestamp_string(time(NULL));
                    ui_print_message_bubble_simple(current_username, chat_input, 1, ts ? ts : "", current_username);
                    if (ts) {
                        free(ts);
                    }
                    
                    // Wait a bit for response
                    #ifdef _WIN32
                    Sleep(100);
                    #else
                    usleep(100000);
                    #endif
                }
                break;
            }
            
            case 12: {  // Search Chat History
                ui_input_prompt("Search keyword");
                fgets(msg.content, sizeof(msg.content), stdin);
                trim_newline(msg.content);
                ui_input_prompt("Recipient (or group ID, leave empty for all)");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_SEARCH_HISTORY;
                send_command(socket, &msg);
                break;
            }
            
            case 13: {  // Create Group
                ui_input_prompt("Group name");
                fgets(msg.content, sizeof(msg.content), stdin);
                trim_newline(msg.content);
                
                msg.cmd = CMD_CREATE_GROUP;
                send_command(socket, &msg);
                break;
            }
            
            case 14: {  // Add User to Group
                ui_input_prompt("Group ID");
                fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                trim_newline(msg.extra_data);
                ui_input_prompt("Username to add");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_ADD_TO_GROUP;
                send_command(socket, &msg);
                break;
            }
            
            case 15: {  // Remove User from Group
                ui_input_prompt("Group ID");
                fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                trim_newline(msg.extra_data);
                ui_input_prompt("Username to remove");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_REMOVE_FROM_GROUP;
                send_command(socket, &msg);
                break;
            }
            
            case 16: {  // Leave Group
                ui_input_prompt("Group ID");
                fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                trim_newline(msg.extra_data);
                
                msg.cmd = CMD_LEAVE_GROUP;
                send_command(socket, &msg);
                break;
            }
            
            case 17: {  // Chat Group (Enter Group Chat Room)
                ui_input_prompt("Group ID");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                // Validate group ID
                if (strlen(msg.recipient) == 0) {
                    ui_print_notification("Group ID cannot be empty", NOTIF_ERROR);
                    clear_stdin_buffer();
                    break;
                }
                
                // Enter group chat room mode
                current_ui_state = UI_STATE_CHAT_GROUP;
                strncpy(current_chat_group, msg.recipient, MAX_GROUP_ID - 1);
                
                // Clear screen and show chat header
                ui_print_chat_header(msg.recipient);
                request_chat_history(msg.recipient, true);
                
                printf("%sLoading group chat history...%s\n\n", COLOR_INFO, COLOR_RESET);
                
                // Enter chat input loop
                char chat_input[BUFFER_SIZE];
                printf("%sType '/back' or '/menu' to return to menu.%s\n\n", COLOR_SYSTEM, COLOR_RESET);
                ui_print_chat_prompt(current_username);
                
                while (current_ui_state == UI_STATE_CHAT_GROUP && is_connected) {
                    if (fgets(chat_input, sizeof(chat_input), stdin) == NULL) {
                        break;
                    }
                    
                    trim_newline(chat_input);
                    
                    // Check for commands
                    if (strcmp(chat_input, "/back") == 0 || strcmp(chat_input, "/menu") == 0) {
                        current_ui_state = UI_STATE_MENU;
                        current_chat_group[0] = '\0';
                        break;
                    }
                    
                    // Validate message
                    if (strlen(chat_input) == 0) {
                        ui_print_chat_prompt(current_username);
                        continue;
                    }
                    
                    if (strlen(chat_input) >= MAX_CONTENT - 1) {
                        printf("%sMessage too long!%s\n", COLOR_ERROR, COLOR_RESET);
                        ui_print_chat_prompt(current_username);
                        continue;
                    }
                    
                    // Send group message
                    ProtocolMessage chat_msg;
                    memset(&chat_msg, 0, sizeof(ProtocolMessage));
                    strncpy(chat_msg.sender, current_username, MAX_USERNAME - 1);
                    strncpy(chat_msg.recipient, current_chat_group, MAX_GROUP_ID - 1);
                    strncpy(chat_msg.content, chat_input, MAX_CONTENT - 1);
                    chat_msg.cmd = CMD_GROUP_MESSAGE;
                    chat_msg.msg_type = MSG_TEXT;
                    
                    send_command(socket, &chat_msg);
                    
                    char* ts = get_timestamp_string(time(NULL));
                    ui_print_message_bubble_simple(current_username, chat_input, 1, ts ? ts : "", current_username);
                    if (ts) {
                        free(ts);
                    }
                    
                    // Wait a bit for response
                    #ifdef _WIN32
                    Sleep(100);
                    #else
                    usleep(100000);
                    #endif
                }
                break;
            }
            
            case 18: {  // Set Group Name / Rename Group
                ui_input_prompt("Group ID");
                fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                trim_newline(msg.extra_data);
                ui_input_prompt("New group name");
                fgets(msg.content, sizeof(msg.content), stdin);
                trim_newline(msg.content);
                
                msg.cmd = CMD_SET_GROUP_NAME;
                send_command(socket, &msg);
                break;
            }
            
            case 19: {  // Pin Message
                ui_input_prompt("Group ID or recipient");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                ui_input_prompt("Message ID to pin");
                fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                trim_newline(msg.extra_data);
                
                msg.cmd = CMD_PIN_MESSAGE;
                send_command(socket, &msg);
                break;
            }
            
            case 20: {  // Get Pinned Messages
                ui_input_prompt("Group ID or recipient");
                fgets(msg.recipient, sizeof(msg.recipient), stdin);
                trim_newline(msg.recipient);
                
                msg.cmd = CMD_GET_PINNED;
                send_command(socket, &msg);
                break;
            }
            
            case 21: {  // Disconnect
                msg.cmd = CMD_DISCONNECT;
                send_command(socket, &msg);
                is_connected = false;
                break;
            }
            
            case 0:  // Exit
                msg.cmd = CMD_DISCONNECT;
                send_command(socket, &msg);
                is_connected = false;
                return;
            
            default:
                printf("Invalid choice\n");
                break;
        }
        
        // Wait a bit for response
        #ifdef _WIN32
        Sleep(100);
        #else
        usleep(100000);  // 100ms
        #endif
        // Response will be handled by receive thread
    }
}

// Main client function
int main(int argc, char* argv[]) {
    // Initialize UI framework (includes UTF-8 setup for Windows)
    ui_init();
    
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    if (init_client(&client_socket, server_ip) < 0) {
        return 1;
    }
    
    // Start receive thread
    #ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receive_thread, 
                                 &client_socket, 0, NULL);
    if (thread == NULL) {
        printf("Failed to create receive thread\n");
        close_socket(client_socket);
        return 1;
    }
    #else
    pthread_t thread;
    if (pthread_create(&thread, NULL, receive_thread, &client_socket) != 0) {
        printf("Failed to create receive thread\n");
        close_socket(client_socket);
        return 1;
    }
    #endif
    
    handle_user_input(client_socket, current_username);
    
    close_socket(client_socket);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return 0;
}

