#include "client.h"  // Includes common.h which has socket libraries
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Socket libraries are included via common.h:
// Windows: winsock2.h, ws2tcpip.h, windows.h
// Linux: sys/socket.h, netinet/in.h, arpa/inet.h, sys/types.h, netdb.h

socket_t client_socket;
bool is_connected = false;
char current_username[MAX_USERNAME] = "";
bool is_logged_in = false;

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
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid address: %s\n", server_ip);
        close_socket(*client_socket);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return -1;
    }

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
        int sent = send(socket, buffer, len, 0);
        if (sent == SOCKET_ERROR) {
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
        case CMD_RECEIVE_MESSAGE:
            printf("\n[Message from %s]: %s\n", msg->sender, msg->content);
            printf("> ");
            fflush(stdout);
            break;
        case CMD_SUCCESS:
            printf("Success: %s\n", msg->content);
            /* Update client login state based on server messages */
            if (strcmp(msg->content, "Login successful") == 0) {
                is_logged_in = true;
            } else if (strcmp(msg->content, "Logged out") == 0) {
                /* Server confirmed logout */
                is_logged_in = false;
                current_username[0] = '\0';
            }
            break;
        case CMD_ERROR:
            printf("Error: %s\n", msg->content);
            if (strcmp(msg->content, "Invalid credentials") == 0) {
                /* clear pending username on failed login */
                current_username[0] = '\0';
                is_logged_in = false;
            }
            break;
        case CMD_GET_FRIENDS:
            printf("%s\n", msg->content);
            break;
        case CMD_SEARCH_HISTORY:
            printf("%s\n", msg->content);
            break;
        case CMD_GET_PINNED:
            printf("%s\n", msg->content);
            break;
        default:
            printf("Response: %s\n", msg->content);
            break;
    }
    
    free(msg);
}

// Receive thread function
void* receive_thread(void* arg) {
    socket_t socket = *(socket_t*)arg;
    while (is_connected) {
        receive_response(socket);
    }
    return NULL;
}

// Print menu (two modes: not-logged-in and logged-in)
void print_menu() {
    printf("\n=== Chat Application Menu ===\n");
    if (!is_logged_in) {
        printf("1. Register\n");
        printf("2. Login\n");
        printf("0. Exit\n");
        printf("Choice: ");
        return;
    }

    // Logged-in menu: replace Register with Logout, keep other features
    printf("1. Logout\n");
    printf("2. Get Friends List\n");
    printf("3. Add Friend\n");
    printf("4. Send Message (1-1)\n");
    printf("5. Create Group\n");
    printf("6. Add User to Group\n");
    printf("7. Remove User from Group\n");
    printf("8. Leave Group\n");
    printf("9. Send Group Message\n");
    printf("10. Search Chat History\n");
    printf("11. Set Group Name\n");
    printf("12. Block User\n");
    printf("13. Unblock User\n");
    printf("14. Pin Message\n");
    printf("15. Get Pinned Messages\n");
    printf("16. Disconnect\n");
    printf("0. Exit\n");
    printf("Choice: ");
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
        /* If the client is logged in use the stored username as sender */
        if (is_logged_in && current_username[0] != '\0') {
            strncpy(msg.sender, current_username, MAX_USERNAME - 1);
        } else {
            strncpy(msg.sender, username, MAX_USERNAME - 1);
        }

        if (!is_logged_in) {
            /* Minimal menu when not logged in */
            switch (choice) {
                case 1: {  // Register
                    char password[MAX_USERNAME];
                    printf("Enter username: ");
                    fgets(msg.sender, sizeof(msg.sender), stdin);
                    trim_newline(msg.sender);
                    printf("Enter password: ");
                    fgets(password, sizeof(password), stdin);
                    trim_newline(password);
                    msg.cmd = CMD_REGISTER;
                    strncpy(msg.content, password, MAX_CONTENT - 1);
                    send_command(socket, &msg);
                    break;
                }
                case 2: {  // Login
                    char password[MAX_USERNAME];
                    printf("Enter username: ");
                    fgets(msg.sender, sizeof(msg.sender), stdin);
                    trim_newline(msg.sender);
                    printf("Enter password: ");
                    fgets(password, sizeof(password), stdin);
                    trim_newline(password);

                    strncpy(current_username, msg.sender, MAX_USERNAME - 1);
                    msg.cmd = CMD_LOGIN;
                    strncpy(msg.content, password, MAX_CONTENT - 1);
                    send_command(socket, &msg);

                    /* Wait for server response (receive_response runs in separate thread)
                       Poll is_logged_in or current_username cleared by receive_response on failure
                       Timeout after ~3 seconds */
                    int waited_ms = 0;
                    const int step_ms = 100; /* 100ms */
                    const int max_wait_ms = 3000; /* 3s */
                    while (waited_ms < max_wait_ms && !is_logged_in && current_username[0] != '\0') {
                        #ifdef _WIN32
                        Sleep(step_ms);
                        #else
                        usleep(step_ms * 1000);
                        #endif
                        waited_ms += step_ms;
                    }

                    if (is_logged_in) {
                        printf("Logged in as %s\n", current_username);
                    } else if (current_username[0] == '\0') {
                        printf("Login failed\n");
                    } else {
                        printf("Login timed out, please try again\n");
                        /* reset pending username */
                        current_username[0] = '\0';
                    }
                    break;
                }
                case 0: { // Exit
                    msg.cmd = CMD_DISCONNECT;
                    send_command(socket, &msg);
                    is_connected = false;
                    return;
                }
                default:
                    printf("Invalid choice\n");
                    break;
            }
        } else {
            /* Logged in: Map choices where 1=Logout, 2.. -> other features (shifted from previous numbering)
               This keeps behaviour similar to earlier menu but forbids registration while logged in */
            switch (choice) {
                case 1: {  // Logout
                    msg.cmd = CMD_LOGOUT;
                    send_command(socket, &msg);
                    break;
                }
                case 2: {
                    msg.cmd = CMD_GET_FRIENDS;
                    send_command(socket, &msg);
                    break;
                }
                case 3: {  // Add Friend
                    printf("Enter username to add as friend: ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    msg.cmd = CMD_ADD_FRIEND;
                    send_command(socket, &msg);
                    break;
                }
                case 4: {  // Send Message
                    printf("Enter recipient username: ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    printf("Enter message: ");
                    fgets(msg.content, sizeof(msg.content), stdin);
                    trim_newline(msg.content);
                    printf("Is emoji? (y/n): ");
                    char emoji_choice = getchar();
                    getchar();  // consume newline
                    msg.msg_type = (emoji_choice == 'y' || emoji_choice == 'Y') ? MSG_EMOJI : MSG_TEXT;
                    msg.cmd = CMD_SEND_MESSAGE;
                    send_command(socket, &msg);
                    break;
                }
                case 5: {  // Create Group
                    printf("Enter group name: ");
                    fgets(msg.content, sizeof(msg.content), stdin);
                    trim_newline(msg.content);
                    msg.cmd = CMD_CREATE_GROUP;
                    send_command(socket, &msg);
                    break;
                }
                case 6: {  // Add to Group
                    printf("Enter group ID: ");
                    fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                    trim_newline(msg.extra_data);
                    printf("Enter username to add: ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    msg.cmd = CMD_ADD_TO_GROUP;
                    send_command(socket, &msg);
                    break;
                }
                case 7: {  // Remove from Group
                    printf("Enter group ID: ");
                    fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                    trim_newline(msg.extra_data);
                    printf("Enter username to remove: ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    msg.cmd = CMD_REMOVE_FROM_GROUP;
                    send_command(socket, &msg);
                    break;
                }
                case 8: {  // Leave Group
                    printf("Enter group ID: ");
                    fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                    trim_newline(msg.extra_data);
                    msg.cmd = CMD_LEAVE_GROUP;
                    send_command(socket, &msg);
                    break;
                }
                case 9: {  // Group Message
                    printf("Enter group ID: ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    printf("Enter message: ");
                    fgets(msg.content, sizeof(msg.content), stdin);
                    trim_newline(msg.content);
                    printf("Is emoji? (y/n): ");
                    char emoji_choice = getchar();
                    getchar();
                    msg.msg_type = (emoji_choice == 'y' || emoji_choice == 'Y') ? MSG_EMOJI : MSG_TEXT;
                    msg.cmd = CMD_GROUP_MESSAGE;
                    send_command(socket, &msg);
                    break;
                }
                case 10: {  // Search History
                    printf("Enter search keyword: ");
                    fgets(msg.content, sizeof(msg.content), stdin);
                    trim_newline(msg.content);
                    printf("Enter recipient (or group ID, leave empty for all): ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    msg.cmd = CMD_SEARCH_HISTORY;
                    send_command(socket, &msg);
                    break;
                }
                case 11: {  // Set Group Name
                    printf("Enter group ID: ");
                    fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                    trim_newline(msg.extra_data);
                    printf("Enter new group name: ");
                    fgets(msg.content, sizeof(msg.content), stdin);
                    trim_newline(msg.content);
                    msg.cmd = CMD_SET_GROUP_NAME;
                    send_command(socket, &msg);
                    break;
                }
                case 12: {  // Block User
                    printf("Enter username to block: ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    msg.cmd = CMD_BLOCK_USER;
                    send_command(socket, &msg);
                    break;
                }
                case 13: {  // Unblock User
                    printf("Enter username to unblock: ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    msg.cmd = CMD_UNBLOCK_USER;
                    send_command(socket, &msg);
                    break;
                }
                case 14: {  // Pin Message
                    printf("Enter group ID or recipient: ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    printf("Enter message ID to pin: ");
                    fgets(msg.extra_data, sizeof(msg.extra_data), stdin);
                    trim_newline(msg.extra_data);
                    msg.cmd = CMD_PIN_MESSAGE;
                    send_command(socket, &msg);
                    break;
                }
                case 15: {  // Get Pinned
                    printf("Enter group ID or recipient: ");
                    fgets(msg.recipient, sizeof(msg.recipient), stdin);
                    trim_newline(msg.recipient);
                    msg.cmd = CMD_GET_PINNED;
                    send_command(socket, &msg);
                    break;
                }
                case 16: {  // Disconnect
                    msg.cmd = CMD_DISCONNECT;
                    send_command(socket, &msg);
                    is_connected = false;
                    break;
                }
                default:
                    printf("Invalid choice\n");
                    break;
            }
        
            // Wait a bit for response (short pause so UI feels responsive)
            #ifdef _WIN32
            Sleep(100);
            #else
            /* usleep takes microseconds */
            usleep(100000); /* 100ms */
            #endif
            // Response will be handled by receive thread
        }
    }
}



// Main client function
int main(int argc, char* argv[]) {
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

