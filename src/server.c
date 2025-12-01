#include "../include/server.h"
#include "../include/common.h"
#include <ctype.h>

ServerState server_state;

// Helper: determine if a stored message belongs to a direct (1-1) chat
// between user_a and user_b. Group messages (stored_type == 2) are excluded.
static bool matches_direct_chat_history(const char* stored_sender,
                                        const char* stored_recipient,
                                        int stored_type,
                                        const char* user_a,
                                        const char* user_b) {
    if (stored_type == 2) {
        return false;  // Skip group messages
    }

    bool a_to_b = (strcmp(stored_sender, user_a) == 0 &&
                   strcmp(stored_recipient, user_b) == 0);
    bool b_to_a = (strcmp(stored_sender, user_b) == 0 &&
                   strcmp(stored_recipient, user_a) == 0);

    return a_to_b || b_to_a;
}

#define ACCOUNT_FILE "../data/account.txt"
int account_count = 0;

int load_accounts(const char *filename){
    FILE *file = fopen(filename, "r");
    if (!file) {
        /* If account file doesn't exist yet, create it so later append works */
        FILE *f = fopen(filename, "a");
        if (!f) {
            perror("Could not create account file");
            return -1;
        }
        fclose(f);
        return 0; /* nothing to load */
    }

    char username[MAX_USERNAME];
    char password[MAX_USERNAME];
    int loaded = 0;

    /* Expect lines in the form: username password\n */
    while (fscanf(file, "%49s %49s", username, password) == 2) {
        if (server_state.user_count >= (int)(sizeof(server_state.users) / sizeof(server_state.users[0]))) {
            /* no more space */
            break;
        }

        /* add_user will avoid duplicates and initialize fields */
        add_user(&server_state, username, password);
        loaded++;
    }

    fclose(file);
    return loaded;
}

int save_account(const char *filename, const char *username, const char *password) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Could not open account file for appending");
        return -1;
    }

    /* Append a single record as "username password" */
    if (fprintf(file, "%s %s\n", username, password) < 0) {
        fclose(file);
        return -1;
    }

    fflush(file);
    fclose(file);
    return 0;
}
// Initialize server socket
int init_server(socket_t* server_socket) {
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return -1;
    }
    #endif

    *server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_socket == INVALID_SOCKET) {
        #ifdef _WIN32
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        #else
        printf("Socket creation failed: %s\n", strerror(errno));
        #endif
        return -1;
    }

    int opt = 1;
    if (setsockopt(*server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        printf("setsockopt failed\n");
        close_socket(*server_socket);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(*server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        #ifdef _WIN32
        printf("Bind failed: %d\n", WSAGetLastError());
        #else
        printf("Bind failed: %s\n", strerror(errno));
        #endif
        close_socket(*server_socket);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return -1;
    }

    if (listen(*server_socket, 10) == SOCKET_ERROR) {
        #ifdef _WIN32
        printf("Listen failed: %d\n", WSAGetLastError());
        #else
        printf("Listen failed: %s\n", strerror(errno));
        #endif
        close_socket(*server_socket);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return -1;
    }

    // Initialize server state
    memset(&server_state, 0, sizeof(ServerState));
    #ifdef _WIN32
    InitializeCriticalSection(&server_state.mutex);
    #else
    pthread_mutex_init(&server_state.mutex, NULL);
    #endif

    // Initialize log mutex for thread-safe logging
    init_log_mutex();
    
    // Initialize messages file mutex for thread-safe message operations
    init_messages_mutex();

    // Load accounts into server_state from persistence file
    int loaded = load_accounts(ACCOUNT_FILE);
    if (loaded < 0) {
        printf("Warning: failed to load accounts from %s\n", ACCOUNT_FILE);
    } else if (loaded > 0) {
        printf("Loaded %d accounts from %s\n", loaded, ACCOUNT_FILE);
    }

    printf("Server started on port %d\n", PORT);
    return 0;
}

// Find user by username
User* find_user(ServerState* state, const char* username) {
    for (int i = 0; i < state->user_count; i++) {
        if (strcmp(state->users[i].username, username) == 0) {
            return &state->users[i];
        }
    }
    return NULL;
}

// Find group by group_id
Group* find_group(ServerState* state, const char* group_id) {
    for (int i = 0; i < state->group_count; i++) {
        if (strcmp(state->groups[i].group_id, group_id) == 0) {
            return &state->groups[i];
        }
    }
    return NULL;
}

// Add new user
void add_user(ServerState* state, const char* username, const char* password) {
    if (find_user(state, username) != NULL) {
        return;  // User already exists
    }
    
    User* new_user = &state->users[state->user_count++];
    strncpy(new_user->username, username, MAX_USERNAME - 1);
    strncpy(new_user->password, password, MAX_USERNAME - 1);
    new_user->is_online = false;
    new_user->socket = INVALID_SOCKET;
    new_user->blocked_count = 0;
    new_user->friend_count = 0;
    new_user->pending_request_count = 0;
    new_user->last_seen = 0;  // 0 means never logged out (or first time)
}

// Send response to client
void send_response(socket_t socket, CommandType cmd, const char* content) {
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    msg.cmd = cmd;
    strncpy(msg.content, content, MAX_CONTENT - 1);
    
    int len;
    char* buffer = serialize_protocol_message(&msg, &len);
    if (buffer) {
        int sent = send_all(socket, buffer, len);
        if (sent == -1) {
            #ifdef _WIN32
            printf("Send failed: %d\n", WSAGetLastError());
            #else
            printf("Send failed: %s\n", strerror(errno));
            #endif
        }
        free(buffer);
    }
}

// Check if user1 has blocked user2
bool is_blocked(User* user, const char* username) {
    for (int i = 0; i < user->blocked_count; i++) {
        if (strcmp(user->blocked_users[i], username) == 0) {
            return true;
        }
    }
    return false;
}

// Check if users are friends
bool are_friends(User* user1, User* user2) {
    for (int i = 0; i < user1->friend_count; i++) {
        if (strcmp(user1->friends[i], user2->username) == 0) {
            return true;
        }
    }
    return false;
}

// Add friend relationship (bidirectional)
void add_friend(User* user1, User* user2) {
    if (are_friends(user1, user2)) return;
    
    strncpy(user1->friends[user1->friend_count++], user2->username, MAX_USERNAME - 1);
    strncpy(user2->friends[user2->friend_count++], user1->username, MAX_USERNAME - 1);
}

// Handle client connection
#ifdef _WIN32
DWORD WINAPI handle_client(LPVOID arg) {
#else
void* handle_client(void* arg) {
#endif
    ClientThreadData* data = (ClientThreadData*)arg;
    socket_t client_socket = data->client_socket;
    ServerState* state = data->server_state;
    char buffer[BUFFER_SIZE];
    User* current_user = NULL;

    printf("Client connected\n");

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            break;
        }

        buffer[bytes_received] = '\0';
        ProtocolMessage* msg = deserialize_protocol_message(buffer, bytes_received);
        if (!msg) continue;

        #ifdef _WIN32
        EnterCriticalSection(&state->mutex);
        #else
        pthread_mutex_lock(&state->mutex);
        #endif

        // Handle commands
        switch (msg->cmd) {
            case CMD_LOGIN: {
                User* user = find_user(state, msg->sender);
                if (user && strcmp(user->password, msg->content) == 0) {
                    // Check if user is already online (single session enforcement)
                    if (user->is_online && user->socket != INVALID_SOCKET) {
                        // Send termination message to old socket
                        send_response(user->socket, CMD_RECEIVE_MESSAGE, "Your session was terminated due to new login");
                        
                        // Shutdown and close old socket
                        #ifdef _WIN32
                        shutdown(user->socket, SD_BOTH);
                        #else
                        shutdown(user->socket, SHUT_RDWR);
                        #endif
                        close_socket(user->socket);
                        
                        // Log session termination
                        log_activity(msg->sender, "SESSION_TERMINATED", "Old session closed by new login");
                    }
                    
                    // Set new session
                    user->is_online = true;
                    user->socket = client_socket;
                    current_user = user;
                    send_response(client_socket, CMD_SUCCESS, "Login successful");
                    log_activity(msg->sender, "LOGIN", "User logged in");
                    
                    // Send offline messages
                    load_and_send_offline_messages(client_socket, msg->sender);
                } else {
                    send_response(client_socket, CMD_ERROR, "Invalid credentials");
                }
                break;
            }
            
            case CMD_REGISTER: {
                if (find_user(state, msg->sender) != NULL) {
                    send_response(client_socket, CMD_ERROR, "Username already exists");
                } else {
                    /* Persist account first so storage reflects the new user */
                    if (save_account(ACCOUNT_FILE, msg->sender, msg->content) != 0) {
                        send_response(client_socket, CMD_ERROR, "Failed to persist account");
                        break;
                    }

                    add_user(state, msg->sender, msg->content);
                    send_response(client_socket, CMD_SUCCESS, "Registration successful");
                    log_activity(msg->sender, "REGISTER", "New user registered");
                }
                break;
            }
            
            case CMD_GET_FRIENDS: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                char friend_list[BUFFER_SIZE] = "Friends List:\n";
                for (int i = 0; i < current_user->friend_count; i++) {
                    User* friend = find_user(state, current_user->friends[i]);
                    if (friend) {
                        char line[300];
                        if (friend->is_online) {
                            snprintf(line, sizeof(line), "  - %s [ONLINE]\n", friend->username);
                        } else {
                            // Get last seen time
                            if (friend->last_seen > 0) {
                                struct tm* timeinfo = localtime(&friend->last_seen);
                                char time_str[20];
                                strftime(time_str, sizeof(time_str), "%H:%M", timeinfo);
                                snprintf(line, sizeof(line), "  - %s [OFFLINE - Last seen: %s]\n", 
                                        friend->username, time_str);
                            } else {
                                snprintf(line, sizeof(line), "  - %s [OFFLINE]\n", friend->username);
                            }
                        }
                        strcat(friend_list, line);
                    }
                }
                send_response(client_socket, CMD_GET_FRIENDS, friend_list);
                log_activity(current_user->username, "GET_FRIENDS", "Retrieved friend list");
                break;
            }
            
            case CMD_ADD_FRIEND: {
                // Legacy: Direct add (for backward compatibility)
                // New flow: Use CMD_SEND_FRIEND_REQUEST
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                User* friend_user = find_user(state, msg->recipient);
                if (!friend_user) {
                    send_response(client_socket, CMD_ERROR, "User not found");
                    break;
                }
                
                if (strcmp(current_user->username, msg->recipient) == 0) {
                    send_response(client_socket, CMD_ERROR, "Cannot add yourself");
                    break;
                }
                
                if (are_friends(current_user, friend_user)) {
                    send_response(client_socket, CMD_ERROR, "Already friends");
                    break;
                }
                
                add_friend(current_user, friend_user);
                send_response(client_socket, CMD_SUCCESS, "Friend added");
                log_activity(current_user->username, "ADD_FRIEND", msg->recipient);
                break;
            }
            
            case CMD_SEND_FRIEND_REQUEST: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                User* friend_user = find_user(state, msg->recipient);
                if (!friend_user) {
                    send_response(client_socket, CMD_ERROR, "User not found");
                    break;
                }
                
                if (strcmp(current_user->username, msg->recipient) == 0) {
                    send_response(client_socket, CMD_ERROR, "Cannot send request to yourself");
                    break;
                }
                
                if (are_friends(current_user, friend_user)) {
                    send_response(client_socket, CMD_ERROR, "Already friends");
                    break;
                }
                
                // Check if request already sent
                bool already_requested = false;
                for (int i = 0; i < friend_user->pending_request_count; i++) {
                    if (strcmp(friend_user->pending_requests[i], current_user->username) == 0) {
                        already_requested = true;
                        break;
                    }
                }
                
                if (already_requested) {
                    send_response(client_socket, CMD_ERROR, "Friend request already sent");
                    break;
                }
                
                // Add to recipient's pending requests
                if (friend_user->pending_request_count < MAX_FRIENDS) {
                    strncpy(friend_user->pending_requests[friend_user->pending_request_count], 
                           current_user->username, MAX_USERNAME - 1);
                    friend_user->pending_request_count++;
                    
                    // Notify recipient if online
                    if (friend_user->is_online && friend_user->socket != INVALID_SOCKET) {
                        ProtocolMessage notif_msg;
                        memset(&notif_msg, 0, sizeof(ProtocolMessage));
                        notif_msg.cmd = CMD_RECEIVE_MESSAGE;
                        notif_msg.msg_type = MSG_SYSTEM;  // Mark as system notification
                        snprintf(notif_msg.content, sizeof(notif_msg.content), 
                                "You have a friend request from %s", current_user->username);
                        // Don't set sender for notifications
                        int len;
                        char* buffer = serialize_protocol_message(&notif_msg, &len);
                        if (buffer) {
                            send_all(friend_user->socket, buffer, len);
                            free(buffer);
                        }
                    }
                    
                    send_response(client_socket, CMD_SUCCESS, "Friend request sent");
                    log_activity(current_user->username, "SEND_FRIEND_REQUEST", msg->recipient);
                } else {
                    send_response(client_socket, CMD_ERROR, "Recipient has too many pending requests");
                }
                break;
            }
            
            case CMD_ACCEPT_FRIEND_REQUEST: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                User* requester = find_user(state, msg->recipient);
                if (!requester) {
                    send_response(client_socket, CMD_ERROR, "User not found");
                    break;
                }
                
                // Check if request exists
                bool request_found = false;
                int request_index = -1;
                for (int i = 0; i < current_user->pending_request_count; i++) {
                    if (strcmp(current_user->pending_requests[i], msg->recipient) == 0) {
                        request_found = true;
                        request_index = i;
                        break;
                    }
                }
                
                if (!request_found) {
                    send_response(client_socket, CMD_ERROR, "Friend request not found");
                    break;
                }
                
                // Remove from pending requests
                for (int i = request_index; i < current_user->pending_request_count - 1; i++) {
                    strcpy(current_user->pending_requests[i], current_user->pending_requests[i + 1]);
                }
                current_user->pending_request_count--;
                
                // Add as friends
                add_friend(current_user, requester);
                add_friend(requester, current_user);
                
                // Notify requester if online
                if (requester->is_online && requester->socket != INVALID_SOCKET) {
                    ProtocolMessage notif_msg;
                    memset(&notif_msg, 0, sizeof(ProtocolMessage));
                    notif_msg.cmd = CMD_RECEIVE_MESSAGE;
                    notif_msg.msg_type = MSG_SYSTEM;  // Mark as system notification
                    snprintf(notif_msg.content, sizeof(notif_msg.content), 
                            "%s accepted your friend request", current_user->username);
                    // Don't set sender for notifications
                    int len;
                    char* buffer = serialize_protocol_message(&notif_msg, &len);
                    if (buffer) {
                        send_all(requester->socket, buffer, len);
                        free(buffer);
                    }
                }
                
                char success_msg[200];
                snprintf(success_msg, sizeof(success_msg), "Friend request from %s accepted", msg->recipient);
                send_response(client_socket, CMD_SUCCESS, success_msg);
                log_activity(current_user->username, "ACCEPT_FRIEND_REQUEST", msg->recipient);
                break;
            }
            
            case CMD_GET_FRIEND_REQUESTS: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                char request_list[BUFFER_SIZE] = "Friend Requests:\n";
                if (current_user->pending_request_count == 0) {
                    strcat(request_list, "  (No pending requests)\n");
                } else {
                    for (int i = 0; i < current_user->pending_request_count; i++) {
                        char line[200];
                        snprintf(line, sizeof(line), "  %d. %s\n", i + 1, current_user->pending_requests[i]);
                        strcat(request_list, line);
                    }
                }
                send_response(client_socket, CMD_GET_FRIEND_REQUESTS, request_list);
                break;
            }
            
            case CMD_REJECT_FRIEND_REQUEST: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                // Check if request exists
                bool request_found = false;
                int request_index = -1;
                for (int i = 0; i < current_user->pending_request_count; i++) {
                    if (strcmp(current_user->pending_requests[i], msg->recipient) == 0) {
                        request_found = true;
                        request_index = i;
                        break;
                    }
                }
                
                if (!request_found) {
                    send_response(client_socket, CMD_ERROR, "Friend request not found");
                    break;
                }
                
                // Remove from pending requests
                for (int i = request_index; i < current_user->pending_request_count - 1; i++) {
                    strcpy(current_user->pending_requests[i], current_user->pending_requests[i + 1]);
                }
                current_user->pending_request_count--;
                
                char success_msg[200];
                snprintf(success_msg, sizeof(success_msg), "Friend request from %s rejected", msg->recipient);
                send_response(client_socket, CMD_SUCCESS, success_msg);
                log_activity(current_user->username, "REJECT_FRIEND_REQUEST", msg->recipient);
                break;
            }
            
            case CMD_UNFRIEND: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                // Edge Case 1: Self-Unfriend
                if (strcmp(current_user->username, msg->recipient) == 0) {
                    send_response(client_socket, CMD_ERROR, "Cannot unfriend yourself");
                    break;
                }
                
                // Edge Case 2: User Not Found
                User* friend_to_remove = find_user(state, msg->recipient);
                if (!friend_to_remove) {
                    send_response(client_socket, CMD_ERROR, "User not found");
                    break;
                }
                
                // Edge Case 3: Not A Friend - Check if they are actually friends
                bool is_friend = are_friends(current_user, friend_to_remove);
                if (!is_friend) {
                    send_response(client_socket, CMD_ERROR, "User is not in your friend list");
                    break;
                }
                
                // Remove from current_user's friends list (bidirectional removal)
                bool removed_from_current = false;
                for (int i = 0; i < current_user->friend_count; i++) {
                    if (strcmp(current_user->friends[i], msg->recipient) == 0) {
                        for (int j = i; j < current_user->friend_count - 1; j++) {
                            strcpy(current_user->friends[j], current_user->friends[j + 1]);
                        }
                        current_user->friend_count--;
                        removed_from_current = true;
                        break;
                    }
                }
                
                // Remove from friend's friends list (bidirectional)
                bool removed_from_friend = false;
                for (int i = 0; i < friend_to_remove->friend_count; i++) {
                    if (strcmp(friend_to_remove->friends[i], current_user->username) == 0) {
                        for (int j = i; j < friend_to_remove->friend_count - 1; j++) {
                            strcpy(friend_to_remove->friends[j], friend_to_remove->friends[j + 1]);
                        }
                        friend_to_remove->friend_count--;
                        removed_from_friend = true;
                        break;
                    }
                }
                
                // Verify removal was successful
                if (removed_from_current && removed_from_friend) {
                    send_response(client_socket, CMD_SUCCESS, "Unfriended successfully");
                    log_activity(current_user->username, "UNFRIEND", msg->recipient);
                } else {
                    // This should not happen if are_friends() works correctly
                    send_response(client_socket, CMD_ERROR, "Failed to remove friend relationship");
                }
                break;
            }
            
            case CMD_SEND_MESSAGE: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                // Validate message content
                size_t content_len = strlen(msg->content);
                if (content_len == 0) {
                    send_response(client_socket, CMD_ERROR, "Message cannot be empty");
                    break;
                }
                if (content_len >= MAX_CONTENT - 1) {
                    char error_msg[200];
                    #ifdef _WIN32
                    snprintf(error_msg, sizeof(error_msg), 
                            "Message is too long (maximum %d characters). Your message has %u characters.", 
                            MAX_CONTENT - 1, (unsigned int)content_len);
                    #else
                    snprintf(error_msg, sizeof(error_msg), 
                            "Message is too long (maximum %d characters). Your message has %zu characters.", 
                            MAX_CONTENT - 1, content_len);
                    #endif
                    send_response(client_socket, CMD_ERROR, error_msg);
                    break;
                }
                
                // Validate recipient
                if (strlen(msg->recipient) == 0) {
                    send_response(client_socket, CMD_ERROR, "Recipient username cannot be empty");
                    break;
                }
                
                User* recipient = find_user(state, msg->recipient);
                if (!recipient) {
                    send_response(client_socket, CMD_ERROR, "Recipient not found");
                    break;
                }
                
                // Only check if recipient blocked sender (not the other way around)
                if (is_blocked(recipient, current_user->username)) {
                    send_response(client_socket, CMD_ERROR, "You are blocked by this user");
                    break;
                }
                
                // Create message
                Message message;
                time_t now = time(NULL);
                #ifdef _WIN32
                snprintf(message.id, sizeof(message.id), "%s_%I64d", current_user->username, (long long)now);
                #else
                snprintf(message.id, sizeof(message.id), "%s_%lld", current_user->username, (long long)now);
                #endif
                strncpy(message.sender, current_user->username, MAX_USERNAME - 1);
                strncpy(message.content, msg->content, MAX_CONTENT - 1);
                message.type = msg->msg_type;
                message.timestamp = time(NULL);
                message.is_pinned = msg->is_pinned;
                
                // Check if recipient is online
                bool recipient_online = (recipient->is_online && recipient->socket != INVALID_SOCKET);
                
                // Save message
                save_message_to_file(current_user->username, msg->recipient, msg->content, false, recipient_online, msg->is_pinned);
                
                // Send to recipient if online
                if (recipient_online) {
                    ProtocolMessage response;
                    memset(&response, 0, sizeof(ProtocolMessage));
                    response.cmd = CMD_RECEIVE_MESSAGE;
                    strncpy(response.sender, current_user->username, MAX_USERNAME - 1);
                    strncpy(response.content, msg->content, MAX_CONTENT - 1);
                    response.msg_type = msg->msg_type;
                    
                    int len;
                    char* resp_buffer = serialize_protocol_message(&response, &len);
                    if (send_all(recipient->socket, resp_buffer, len) == -1) {
                        #ifdef _WIN32
                        printf("Failed to send message to %s: %d\n", msg->recipient, WSAGetLastError());
                        #else
                        printf("Failed to send message to %s: %s\n", msg->recipient, strerror(errno));
                        #endif
                    }
                    free(resp_buffer);
                }
                
                send_response(client_socket, CMD_SUCCESS, "Message sent");
                log_activity(current_user->username, "SEND_MESSAGE", msg->recipient);
                break;
            }
            
            case CMD_DISCONNECT: {
                if (current_user) {
                    current_user->is_online = false;
                    current_user->socket = INVALID_SOCKET;
                    log_activity(current_user->username, "DISCONNECT", "User disconnected");
                    
                    // Notify friends
                    broadcast_to_friends(state, current_user->username, "User went offline");
                }
                #ifdef _WIN32
                LeaveCriticalSection(&state->mutex);
                #else
                pthread_mutex_unlock(&state->mutex);
                #endif
                close_socket(client_socket);
                free(msg);
                free(data);
                #ifdef _WIN32
                return 0;
                #else
                return NULL;
                #endif
            }
            
            case CMD_CREATE_GROUP: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                char group_id[MAX_GROUP_ID];
                time_t now = time(NULL);
                #ifdef _WIN32
                snprintf(group_id, sizeof(group_id), "GROUP_%s_%I64d", current_user->username, (long long)now);
                #else
                snprintf(group_id, sizeof(group_id), "GROUP_%s_%lld", current_user->username, (long long)now);
                #endif
                
                Group* new_group = &state->groups[state->group_count++];
                strncpy(new_group->group_id, group_id, MAX_GROUP_ID - 1);
                strncpy(new_group->name, msg->content, MAX_GROUP_NAME - 1);
                strncpy(new_group->creator, current_user->username, MAX_USERNAME - 1);
                new_group->member_count = 1;
                strncpy(new_group->members[0], current_user->username, MAX_USERNAME - 1);
                new_group->admin_count = 1;
                strncpy(new_group->admins[0], current_user->username, MAX_USERNAME - 1);
                new_group->message_count = 0;
                new_group->created_at = time(NULL);
                
                char response[200];
                snprintf(response, sizeof(response), "Group created: %s", group_id);
                send_response(client_socket, CMD_SUCCESS, response);
                log_activity(current_user->username, "CREATE_GROUP", group_id);
                break;
            }
            
            case CMD_ADD_TO_GROUP: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                Group* group = find_group(state, msg->extra_data);
                if (!group) {
                    send_response(client_socket, CMD_ERROR, "Group not found");
                    break;
                }
                
                // Check if user is admin
                bool is_admin = false;
                for (int i = 0; i < group->admin_count; i++) {
                    if (strcmp(group->admins[i], current_user->username) == 0) {
                        is_admin = true;
                        break;
                    }
                }
                
                if (!is_admin) {
                    send_response(client_socket, CMD_ERROR, "Not an admin");
                    break;
                }
                
                User* new_member = find_user(state, msg->recipient);
                if (!new_member) {
                    send_response(client_socket, CMD_ERROR, "User not found");
                    break;
                }
                
                // Check if already member
                bool already_member = false;
                for (int i = 0; i < group->member_count; i++) {
                    if (strcmp(group->members[i], msg->recipient) == 0) {
                        already_member = true;
                        break;
                    }
                }
                
                if (!already_member) {
                    strncpy(group->members[group->member_count++], msg->recipient, MAX_USERNAME - 1);
                    send_response(client_socket, CMD_SUCCESS, "User added to group");
                    log_activity(current_user->username, "ADD_TO_GROUP", msg->recipient);
                } else {
                    send_response(client_socket, CMD_ERROR, "User already in group");
                }
                break;
            }
            
            case CMD_REMOVE_FROM_GROUP: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                Group* group = find_group(state, msg->extra_data);
                if (!group) {
                    send_response(client_socket, CMD_ERROR, "Group not found");
                    break;
                }
                
                // Check if user is admin
                bool is_admin = false;
                for (int i = 0; i < group->admin_count; i++) {
                    if (strcmp(group->admins[i], current_user->username) == 0) {
                        is_admin = true;
                        break;
                    }
                }
                
                if (!is_admin) {
                    send_response(client_socket, CMD_ERROR, "Not an admin");
                    break;
                }
                
                // Remove member
                for (int i = 0; i < group->member_count; i++) {
                    if (strcmp(group->members[i], msg->recipient) == 0) {
                        for (int j = i; j < group->member_count - 1; j++) {
                            strcpy(group->members[j], group->members[j + 1]);
                        }
                        group->member_count--;
                        send_response(client_socket, CMD_SUCCESS, "User removed from group");
                        log_activity(current_user->username, "REMOVE_FROM_GROUP", msg->recipient);
                        break;
                    }
                }
                break;
            }
            
            case CMD_LEAVE_GROUP: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                Group* group = find_group(state, msg->extra_data);
                if (!group) {
                    send_response(client_socket, CMD_ERROR, "Group not found");
                    break;
                }
                
                // Remove from group
                for (int i = 0; i < group->member_count; i++) {
                    if (strcmp(group->members[i], current_user->username) == 0) {
                        for (int j = i; j < group->member_count - 1; j++) {
                            strcpy(group->members[j], group->members[j + 1]);
                        }
                        group->member_count--;
                        send_response(client_socket, CMD_SUCCESS, "Left group");
                        log_activity(current_user->username, "LEAVE_GROUP", msg->extra_data);
                        break;
                    }
                }
                break;
            }
            
            case CMD_GROUP_MESSAGE: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                // Validate message content
                size_t content_len = strlen(msg->content);
                if (content_len == 0) {
                    send_response(client_socket, CMD_ERROR, "Message cannot be empty");
                    break;
                }
                if (content_len >= MAX_CONTENT - 1) {
                    char error_msg[200];
                    #ifdef _WIN32
                    snprintf(error_msg, sizeof(error_msg), 
                            "Message is too long (maximum %d characters). Your message has %u characters.", 
                            MAX_CONTENT - 1, (unsigned int)content_len);
                    #else
                    snprintf(error_msg, sizeof(error_msg), 
                            "Message is too long (maximum %d characters). Your message has %zu characters.", 
                            MAX_CONTENT - 1, content_len);
                    #endif
                    send_response(client_socket, CMD_ERROR, error_msg);
                    break;
                }
                
                // Validate group ID
                if (strlen(msg->recipient) == 0) {
                    send_response(client_socket, CMD_ERROR, "Group ID cannot be empty");
                    break;
                }
                
                Group* group = find_group(state, msg->recipient);
                if (!group) {
                    send_response(client_socket, CMD_ERROR, "Group not found");
                    break;
                }
                
                // Check if user is member
                bool is_member = false;
                for (int i = 0; i < group->member_count; i++) {
                    if (strcmp(group->members[i], current_user->username) == 0) {
                        is_member = true;
                        break;
                    }
                }
                
                if (!is_member) {
                    send_response(client_socket, CMD_ERROR, "Not a member");
                    break;
                }
                
                // Add message to group
                Message* group_msg = &group->messages[group->message_count++];
                time_t now = time(NULL);
                #ifdef _WIN32
                snprintf(group_msg->id, sizeof(group_msg->id), "%s_%I64d", current_user->username, (long long)now);
                #else
                snprintf(group_msg->id, sizeof(group_msg->id), "%s_%lld", current_user->username, (long long)now);
                #endif
                strncpy(group_msg->sender, current_user->username, MAX_USERNAME - 1);
                strncpy(group_msg->content, msg->content, MAX_CONTENT - 1);
                group_msg->type = msg->msg_type;
                group_msg->timestamp = time(NULL);
                group_msg->is_pinned = msg->is_pinned;
                
                // For group messages, save for each member with their online status
                for (int i = 0; i < group->member_count; i++) {
                    User* member = find_user(state, group->members[i]);
                    if (member) {
                        bool member_online = (member->is_online && member->socket != INVALID_SOCKET);
                        // Save message for this member (use member username as recipient for offline message tracking)
                        save_message_to_file(current_user->username, group->members[i], msg->content, true, member_online, msg->is_pinned);
                    }
                }
                
                // Broadcast to all online members
                ProtocolMessage response;
                memset(&response, 0, sizeof(ProtocolMessage));
                response.cmd = CMD_RECEIVE_MESSAGE;
                strncpy(response.sender, current_user->username, MAX_USERNAME - 1);
                strncpy(response.recipient, msg->recipient, MAX_USERNAME - 1);
                strncpy(response.content, msg->content, MAX_CONTENT - 1);
                response.msg_type = msg->msg_type;
                
                int len;
                char* resp_buffer = serialize_protocol_message(&response, &len);
                
                for (int i = 0; i < group->member_count; i++) {
                    User* member = find_user(state, group->members[i]);
                    if (member && member->is_online && member->socket != INVALID_SOCKET && 
                        strcmp(member->username, current_user->username) != 0) {
                        if (send_all(member->socket, resp_buffer, len) == -1) {
                            #ifdef _WIN32
                            printf("Failed to send group message to %s: %d\n", member->username, WSAGetLastError());
                            #else
                            printf("Failed to send group message to %s: %s\n", member->username, strerror(errno));
                            #endif
                        }
                    }
                }
                free(resp_buffer);
                
                send_response(client_socket, CMD_SUCCESS, "Group message sent");
                log_activity(current_user->username, "GROUP_MESSAGE", msg->recipient);
                break;
            }
            
            case CMD_GET_CHAT_HISTORY: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                if (strlen(msg->recipient) == 0) {
                    send_response(client_socket, CMD_ERROR, "Recipient required");
                    break;
                }
                
                const int MAX_HISTORY_ENTRIES = 20;
                bool is_group_request = (strncmp(msg->recipient, "GROUP_", 6) == 0);
                char response[BUFFER_SIZE];
                response[0] = '\0';
                
                if (is_group_request) {
                    Group* group = find_group(state, msg->recipient);
                    if (!group) {
                        send_response(client_socket, CMD_ERROR, "Group not found");
                        break;
                    }
                    
                    bool is_member = false;
                    for (int i = 0; i < group->member_count; i++) {
                        if (strcmp(group->members[i], current_user->username) == 0) {
                            is_member = true;
                            break;
                        }
                    }
                    if (!is_member) {
                        send_response(client_socket, CMD_ERROR, "Not a member of this group");
                        break;
                    }
                    
                    int start_idx = group->message_count > MAX_HISTORY_ENTRIES ? group->message_count - MAX_HISTORY_ENTRIES : 0;
                    for (int i = start_idx; i < group->message_count; i++) {
                        char* ts = get_timestamp_string(group->messages[i].timestamp);
                        if (!ts) continue;
                        char line_buffer[512];
                        snprintf(line_buffer, sizeof(line_buffer), "%s|%s|%s\n",
                                 ts,
                                 group->messages[i].sender,
                                 group->messages[i].content);
                        free(ts);
                        if (strlen(response) + strlen(line_buffer) >= sizeof(response) - 1) {
                            break;
                        }
                        strcat(response, line_buffer);
                    }
                    
                    if (strlen(response) == 0) {
                        snprintf(response, sizeof(response), "NO_HISTORY");
                    }
                    
                    send_response(client_socket, CMD_GET_CHAT_HISTORY, response);
                    break;
                }
                
                bool history_found = false;
                
                lock_messages_mutex();
                FILE* file = fopen("../data/messages.txt", "r");
                if (file) {
                    char line[BUFFER_SIZE];
                    while (fgets(line, sizeof(line), file)) {
                        size_t len = strlen(line);
                        if (len > 0 && line[len - 1] == '\n') {
                            line[len - 1] = '\0';
                        }
                        
                        char* tokens[7];
                        char* line_copy = (char*)malloc(strlen(line) + 1);
                        if (!line_copy) {
                            continue;
                        }
                        strcpy(line_copy, line);
                        
                        int token_count = 0;
                        char* token = strtok(line_copy, "|");
                        while (token && token_count < 7) {
                            tokens[token_count++] = token;
                            token = strtok(NULL, "|");
                        }
                        
                        if (token_count == 7) {
                            char* timestamp = tokens[0];
                            char* sender = tokens[1];
                            char* recipient = tokens[2];
                            int type = atoi(tokens[3]);
                            char* content = tokens[4];
                            
                            bool match = matches_direct_chat_history(
                                sender,
                                recipient,
                                type,
                                current_user->username,
                                msg->recipient
                            );
                            
                            if (match) {
                                char line_buffer[512];
                                int written = snprintf(line_buffer, sizeof(line_buffer), "%s|%s|%s\n",
                                                       timestamp, sender, content);
                                if (written > 0 &&
                                    (size_t)(written + strlen(response)) < sizeof(response) - 1) {
                                    strncat(response, line_buffer, sizeof(response) - strlen(response) - 1);
                                    history_found = true;
                                }
                            }
                        }
                        
                        free(line_copy);
                    }
                    fclose(file);
                }
                unlock_messages_mutex();
                
                if (!history_found) {
                    snprintf(response, sizeof(response), "NO_HISTORY");
                }
                
                send_response(client_socket, CMD_GET_CHAT_HISTORY, response);
                break;
            }
            
            case CMD_SEARCH_HISTORY: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                int result_count = 0;
                char** results = search_messages(msg->content, current_user->username, msg->recipient, &result_count);
                
                char response[BUFFER_SIZE] = "Search results: ";
                for (int i = 0; i < result_count && i < 10; i++) {
                    strcat(response, results[i]);
                    strcat(response, " | ");
                    free(results[i]);
                }
                free(results);
                
                send_response(client_socket, CMD_SEARCH_HISTORY, response);
                log_activity(current_user->username, "SEARCH_HISTORY", msg->content);
                break;
            }
            
            case CMD_SET_GROUP_NAME: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                Group* group = find_group(state, msg->extra_data);
                if (!group) {
                    send_response(client_socket, CMD_ERROR, "Group not found");
                    break;
                }
                
                // Check if user is admin
                bool is_admin = false;
                for (int i = 0; i < group->admin_count; i++) {
                    if (strcmp(group->admins[i], current_user->username) == 0) {
                        is_admin = true;
                        break;
                    }
                }
                
                if (is_admin) {
                    strncpy(group->name, msg->content, MAX_GROUP_NAME - 1);
                    send_response(client_socket, CMD_SUCCESS, "Group name updated");
                    log_activity(current_user->username, "SET_GROUP_NAME", msg->extra_data);
                } else {
                    send_response(client_socket, CMD_ERROR, "Not an admin");
                }
                break;
            }
            
            case CMD_BLOCK_USER: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                if (strcmp(current_user->username, msg->recipient) == 0) {
                    send_response(client_socket, CMD_ERROR, "Cannot block yourself");
                    break;
                }
                
                if (is_blocked(current_user, msg->recipient)) {
                    send_response(client_socket, CMD_ERROR, "User already blocked");
                    break;
                }
                
                strncpy(current_user->blocked_users[current_user->blocked_count++], 
                       msg->recipient, MAX_USERNAME - 1);
                send_response(client_socket, CMD_SUCCESS, "User blocked");
                log_activity(current_user->username, "BLOCK_USER", msg->recipient);
                break;
            }
            
            case CMD_UNBLOCK_USER: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                for (int i = 0; i < current_user->blocked_count; i++) {
                    if (strcmp(current_user->blocked_users[i], msg->recipient) == 0) {
                        for (int j = i; j < current_user->blocked_count - 1; j++) {
                            strcpy(current_user->blocked_users[j], current_user->blocked_users[j + 1]);
                        }
                        current_user->blocked_count--;
                        send_response(client_socket, CMD_SUCCESS, "User unblocked");
                        log_activity(current_user->username, "UNBLOCK_USER", msg->recipient);
                        break;
                    }
                }
                break;
            }
            
            case CMD_PIN_MESSAGE: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                // Find and pin message in group or conversation
                if (strncmp(msg->recipient, "GROUP_", 6) == 0) {
                    Group* group = find_group(state, msg->recipient);
                    if (group) {
                        for (int i = 0; i < group->message_count; i++) {
                            if (strcmp(group->messages[i].id, msg->extra_data) == 0) {
                                group->messages[i].is_pinned = true;
                                
                                // Update pin status in file for persistence
                                char* timestamp_str = get_timestamp_string(group->messages[i].timestamp);
                                if (timestamp_str) {
                                    update_message_pin_status_in_file(
                                        group->messages[i].sender,
                                        group->messages[i].content,
                                        timestamp_str,
                                        1  // is_pinned = true
                                    );
                                    free(timestamp_str);
                                }
                                
                                send_response(client_socket, CMD_SUCCESS, "Message pinned");
                                log_activity(current_user->username, "PIN_MESSAGE", msg->extra_data);
                                break;
                            }
                        }
                    }
                } else {
                    // For 1-1 chat, search in messages.txt and update pin status
                    // For now, we'll try to find by content if extra_data is provided
                    if (strlen(msg->extra_data) > 0) {
                        // Try to extract timestamp and content from extra_data or use a different approach
                        // This is a simplified version - in production, you might want to store message IDs
                        // For 1-1 messages, we can search by sender, recipient, and content
                        // Note: This requires the client to send the message content or timestamp
                        // For now, we'll update based on the most recent message matching the criteria
                        lock_messages_mutex();
                        FILE* file = fopen("../data/messages.txt", "r");
                        if (file) {
                            char line[BUFFER_SIZE];
                            char last_matching_line[BUFFER_SIZE] = "";
                            char last_timestamp[64] = "";
                            char last_sender[MAX_USERNAME] = "";
                            char last_content[MAX_CONTENT] = "";
                            
                            while (fgets(line, sizeof(line), file)) {
                                size_t len = strlen(line);
                                if (len > 0 && line[len - 1] == '\n') {
                                    line[len - 1] = '\0';
                                }
                                
                                char* tokens[7];
                                char* line_copy = (char*)malloc(strlen(line) + 1);
                                if (!line_copy) continue;
                                strcpy(line_copy, line);
                                
                                int token_count = 0;
                                char* token = strtok(line_copy, "|");
                                while (token && token_count < 7) {
                                    tokens[token_count++] = token;
                                    token = strtok(NULL, "|");
                                }
                                
                                if (token_count == 7) {
                                    // Check if this is a 1-1 message between current_user and recipient
                                    bool is_1v1 = (atoi(tokens[3]) == 0);  // TYPE 0 = 1-1 message
                                    bool involves_users = ((strcmp(tokens[1], current_user->username) == 0 && 
                                                           strcmp(tokens[2], msg->recipient) == 0) ||
                                                          (strcmp(tokens[1], msg->recipient) == 0 && 
                                                           strcmp(tokens[2], current_user->username) == 0));
                                    
                                    if (is_1v1 && involves_users) {
                                        strcpy(last_matching_line, line);
                                        strncpy(last_timestamp, tokens[0], sizeof(last_timestamp) - 1);
                                        strncpy(last_sender, tokens[1], sizeof(last_sender) - 1);
                                        strncpy(last_content, tokens[4], sizeof(last_content) - 1);
                                    }
                                }
                                free(line_copy);
                            }
                            fclose(file);
                            unlock_messages_mutex();
                            
                            // Update the most recent matching message
                            if (strlen(last_timestamp) > 0) {
                                update_message_pin_status_in_file(last_sender, last_content, last_timestamp, 1);
                            }
                        } else {
                            unlock_messages_mutex();
                        }
                    }
                }
                break;
            }
            
            case CMD_GET_PINNED: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                char pinned_list[BUFFER_SIZE] = "Pinned messages: ";
                if (strncmp(msg->recipient, "GROUP_", 6) == 0) {
                    Group* group = find_group(state, msg->recipient);
                    if (group) {
                        for (int i = 0; i < group->message_count; i++) {
                            if (group->messages[i].is_pinned) {
                                strcat(pinned_list, group->messages[i].content);
                                strcat(pinned_list, " | ");
                            }
                        }
                    }
                }
                send_response(client_socket, CMD_GET_PINNED, pinned_list);
                break;
            }
            
            default:
                send_response(client_socket, CMD_ERROR, "Unknown command");
                break;
        }

        #ifdef _WIN32
        LeaveCriticalSection(&state->mutex);
        #else
        pthread_mutex_unlock(&state->mutex);
        #endif

        free(msg);
    }

    if (current_user) {
        current_user->is_online = false;
        current_user->socket = INVALID_SOCKET;
        // Update last_seen when user disconnects
        current_user->last_seen = time(NULL);
        log_activity(current_user->username, "LOGOUT", "User logged out (connection closed)");
        
        // Notify users who recently chatted with this user (disconnect alert)
        notify_recent_chatters(state, current_user->username);
        
        // Send specific disconnect notification to friends who are online
        broadcast_to_friends(state, current_user->username, "User has disconnected");
    }
    
    close_socket(client_socket);
    free(data);
    #ifdef _WIN32
    return 0;
    #else
    return NULL;
    #endif
}

// Notify users who recently chatted with disconnected user
void notify_recent_chatters(ServerState* state, const char* disconnected_username) {
    // Read messages.txt to find recent chatters
    lock_messages_mutex();
    FILE* file = fopen("../data/messages.txt", "r");
    if (!file) {
        unlock_messages_mutex();
        return;
    }
    
    char line[BUFFER_SIZE];
    char recent_chatters[MAX_FRIENDS][MAX_USERNAME];
    int chatter_count = 0;
    
    // Find users who chatted with disconnected user (check last 100 messages)
    int line_count = 0;
    while (fgets(line, sizeof(line), file) && line_count < 100 && chatter_count < MAX_FRIENDS) {
        line_count++;
        // Parse: TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED
        char* tokens[7];
        char* line_copy = (char*)malloc(strlen(line) + 1);
        strcpy(line_copy, line);
        
        int token_count = 0;
        char* token = strtok(line_copy, "|");
        while (token && token_count < 7) {
            tokens[token_count++] = token;
            token = strtok(NULL, "|");
        }
        
        if (token_count >= 3) {
            // Check if message involves disconnected user
            if (strcmp(tokens[1], disconnected_username) == 0) {
                // Disconnected user is sender, notify recipient
                bool already_added = false;
                for (int i = 0; i < chatter_count; i++) {
                    if (strcmp(recent_chatters[i], tokens[2]) == 0) {
                        already_added = true;
                        break;
                    }
                }
                if (!already_added && strcmp(tokens[2], disconnected_username) != 0) {
                    strncpy(recent_chatters[chatter_count++], tokens[2], MAX_USERNAME - 1);
                }
            } else if (strcmp(tokens[2], disconnected_username) == 0) {
                // Disconnected user is recipient, notify sender
                bool already_added = false;
                for (int i = 0; i < chatter_count; i++) {
                    if (strcmp(recent_chatters[i], tokens[1]) == 0) {
                        already_added = true;
                        break;
                    }
                }
                if (!already_added && strcmp(tokens[1], disconnected_username) != 0) {
                    strncpy(recent_chatters[chatter_count++], tokens[1], MAX_USERNAME - 1);
                }
            }
        }
        
        free(line_copy);
    }
    
    fclose(file);
    unlock_messages_mutex();
    
    // Notify each recent chatter
    for (int i = 0; i < chatter_count; i++) {
        User* chatter = find_user(state, recent_chatters[i]);
        if (chatter && chatter->is_online && chatter->socket != INVALID_SOCKET) {
            ProtocolMessage notif_msg;
            memset(&notif_msg, 0, sizeof(ProtocolMessage));
            notif_msg.cmd = CMD_RECEIVE_MESSAGE;
            notif_msg.msg_type = MSG_SYSTEM;
            char notification[200];
            snprintf(notification, sizeof(notification), 
                    "Người dùng %s đã rời cuộc trò chuyện", disconnected_username);
            strncpy(notif_msg.content, notification, MAX_CONTENT - 1);
            
            int len;
            char* buffer = serialize_protocol_message(&notif_msg, &len);
            if (buffer) {
                send_all(chatter->socket, buffer, len);
                free(buffer);
            }
        }
    }
}

// Broadcast message to all friends
void broadcast_to_friends(ServerState* state, const char* username, const char* message) {
    User* user = find_user(state, username);
    if (!user) return;
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    msg.cmd = CMD_RECEIVE_MESSAGE;
    strncpy(msg.sender, username, MAX_USERNAME - 1);
    strncpy(msg.content, message, MAX_CONTENT - 1);
    msg.msg_type = MSG_SYSTEM;
    
    int len;
    char* buffer = serialize_protocol_message(&msg, &len);
    
    for (int i = 0; i < user->friend_count; i++) {
        User* friend = find_user(state, user->friends[i]);
        if (friend && friend->is_online && friend->socket != INVALID_SOCKET) {
            if (send_all(friend->socket, buffer, len) == -1) {
                #ifdef _WIN32
                printf("Failed to broadcast to %s: %d\n", friend->username, WSAGetLastError());
                #else
                printf("Failed to broadcast to %s: %s\n", friend->username, strerror(errno));
                #endif
            }
        }
    }
    
    free(buffer);
}

// Save message to file
void save_message_to_file(const char* sender, const char* recipient, const char* content, bool is_group, bool recipient_online, bool is_pinned) {
    lock_messages_mutex();
    FILE* file = fopen("../data/messages.txt", "a");
    if (file) {
        time_t now = time(NULL);
        char* time_str = get_timestamp_string(now);
        // Format: TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED
        // TYPE: 0=MSG_TEXT (1-1), 1=MSG_EMOJI, 2=GROUP
        int msg_type = is_group ? 2 : 0;  // For now, we use 0 for 1-1, 2 for GROUP
        fprintf(file, "%s|%s|%s|%d|%s|%d|%d\n", 
                time_str, sender, recipient, 
                msg_type,  // TYPE: 0=1-1, 2=GROUP
                content, 
                recipient_online ? 1 : 0,  // DELIVERED: 0=offline, 1=online
                is_pinned ? 1 : 0);  // PINNED: 0=no, 1=yes
        fclose(file);
        free(time_str);
    }
    unlock_messages_mutex();
}

// Update message pin status in messages.txt file (persistence)
void update_message_pin_status_in_file(const char* sender, const char* content, const char* timestamp, int is_pinned) {
    lock_messages_mutex();
    
    FILE* file_in = fopen("../data/messages.txt", "r");
    if (!file_in) {
        unlock_messages_mutex();
        return;  // File doesn't exist yet, nothing to update
    }
    
    FILE* file_out = fopen("../data/messages.tmp", "w");
    if (!file_out) {
        fclose(file_in);
        unlock_messages_mutex();
        return;  // Cannot create temp file
    }
    
    char line[BUFFER_SIZE];
    bool found = false;
    
    // Read all lines and update matching message
    while (fgets(line, sizeof(line), file_in)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Parse format: TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED
        char* tokens[7];
        char* line_copy = (char*)malloc(strlen(line) + 1);
        if (!line_copy) {
            fprintf(file_out, "%s\n", line);
            continue;
        }
        strcpy(line_copy, line);
        
        int token_count = 0;
        char* token = strtok(line_copy, "|");
        while (token && token_count < 7) {
            tokens[token_count++] = token;
            token = strtok(NULL, "|");
        }
        
        if (token_count == 7) {
            char* msg_timestamp = tokens[0];
            char* msg_sender = tokens[1];
            char* msg_recipient = tokens[2];
            char* msg_type = tokens[3];
            char* msg_content = tokens[4];
            char* msg_delivered = tokens[5];
            // msg_pinned = tokens[6] - not needed here, we'll update it
            
            // Check if this line matches the message to pin/unpin
            bool matches = (strcmp(msg_timestamp, timestamp) == 0) &&
                          (strcmp(msg_sender, sender) == 0) &&
                          (strcmp(msg_content, content) == 0);
            
            if (matches && !found) {
                // Update PINNED status
                fprintf(file_out, "%s|%s|%s|%s|%s|%s|%d\n",
                        msg_timestamp, msg_sender, msg_recipient, msg_type,
                        msg_content, msg_delivered, is_pinned ? 1 : 0);
                found = true;
            } else {
                // Keep original line
                fprintf(file_out, "%s\n", line);
            }
        } else {
            // Keep original line if format is invalid
            fprintf(file_out, "%s\n", line);
        }
        
        free(line_copy);
    }
    
    fclose(file_in);
    fclose(file_out);
    
    if (found) {
        #ifdef _WIN32
        remove("../data/messages.txt");
        rename("../data/messages.tmp", "../data/messages.txt");
        #else
        remove("../data/messages.txt");
        rename("../data/messages.tmp", "../data/messages.txt");
        #endif
    } else {
        // No match found, delete temp file
        remove("../data/messages.tmp");
    }
    
    unlock_messages_mutex();
}

// Load and send offline messages to user
void load_and_send_offline_messages(socket_t socket, const char* username) {
    lock_messages_mutex();
    
    FILE* file = fopen("../data/messages.txt", "r");
    if (!file) {
        unlock_messages_mutex();
        return;
    }
    
    char line[BUFFER_SIZE];
    char** lines_to_keep = (char**)malloc(10000 * sizeof(char*));
    int lines_count = 0;
    int messages_sent = 0;
    
    // Read all lines and process
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Parse format: TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED
        char* tokens[7];
        char* line_copy = (char*)malloc(strlen(line) + 1);
        strcpy(line_copy, line);
        
        int token_count = 0;
        char* token = strtok(line_copy, "|");
        while (token && token_count < 7) {
            tokens[token_count++] = token;
            token = strtok(NULL, "|");
        }
        
        if (token_count == 7) {
            char* timestamp = tokens[0];
            char* sender = tokens[1];
            char* recipient = tokens[2];
            // int type = atoi(tokens[3]);
            char* content = tokens[4];
            int delivered = atoi(tokens[5]);
            // int pinned = atoi(tokens[6]);
            
            // Check if this message is for this user and not delivered
            if (strcmp(recipient, username) == 0 && delivered == 0) {
                // Send message to user
                char message_content[BUFFER_SIZE];
                snprintf(message_content, sizeof(message_content), "[%s] From %s: %s", 
                        timestamp, sender, content);
                
                ProtocolMessage msg;
                memset(&msg, 0, sizeof(ProtocolMessage));
                msg.cmd = CMD_RECEIVE_MESSAGE;
                strncpy(msg.sender, sender, MAX_USERNAME - 1);
                strncpy(msg.content, message_content, MAX_CONTENT - 1);
                msg.msg_type = MSG_TEXT;
                
                int len;
                char* buffer = serialize_protocol_message(&msg, &len);
                if (buffer) {
                    if (send_all(socket, buffer, len) != -1) {
                        messages_sent++;
                        // Mark as delivered: update DELIVERED to 1
                        char updated_line[BUFFER_SIZE];
                        snprintf(updated_line, sizeof(updated_line), "%s|%s|%s|%s|%s|1|%s",
                                timestamp, sender, recipient, tokens[3], content, tokens[6]);
                        lines_to_keep[lines_count] = (char*)malloc(strlen(updated_line) + 1);
                        strcpy(lines_to_keep[lines_count], updated_line);
                        lines_count++;
                        free(buffer);
                        free(line_copy);
                        continue;
                    }
                    free(buffer);
                }
            }
        }
        
        // Keep original line
        lines_to_keep[lines_count] = (char*)malloc(strlen(line) + 1);
        strcpy(lines_to_keep[lines_count], line);
        lines_count++;
        free(line_copy);
    }
    
    fclose(file);
    
    if (messages_sent > 0) {
        file = fopen("../data/messages.txt", "w");
        if (file) {
            for (int i = 0; i < lines_count; i++) {
                fprintf(file, "%s\n", lines_to_keep[i]);
                free(lines_to_keep[i]);
            }
            fclose(file);
        }
    } else {
        // Free memory if no messages sent
        for (int i = 0; i < lines_count; i++) {
            free(lines_to_keep[i]);
        }
    }
    
    free(lines_to_keep);
    unlock_messages_mutex();
}

// Search messages
char** search_messages(const char* keyword, const char* username, const char* recipient, int* result_count) {
    char** results = (char**)malloc(100 * sizeof(char*));
    *result_count = 0;
    
    lock_messages_mutex();
    FILE* file = fopen("../data/messages.txt", "r");
    if (!file) {
        unlock_messages_mutex();
        return results;
    }
    
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file) && *result_count < 100) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Parse format: TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED
        char* tokens[7];
        char* line_copy = (char*)malloc(strlen(line) + 1);
        strcpy(line_copy, line);
        
        int token_count = 0;
        char* token = strtok(line_copy, "|");
        while (token && token_count < 7) {
            tokens[token_count++] = token;
            token = strtok(NULL, "|");
        }
        
        if (token_count == 7) {
            char* timestamp = tokens[0];
            char* sender = tokens[1];
            char* msg_recipient = tokens[2];
            // int type = atoi(tokens[3]);
            char* content = tokens[4];
            // int delivered = atoi(tokens[5]);
            int pinned = atoi(tokens[6]);
            
            // Check if keyword is in content
            if (strstr(content, keyword) != NULL) {
                // Check if message is relevant to this user
                bool is_relevant = false;
                if (strcmp(sender, username) == 0 || strcmp(msg_recipient, username) == 0) {
                    if (strlen(recipient) == 0 || strcmp(sender, recipient) == 0 || strcmp(msg_recipient, recipient) == 0) {
                        is_relevant = true;
                    }
                }
                
                if (is_relevant) {
                    // Format: [TIMESTAMP] SENDER -> RECIPIENT: CONTENT [PINNED]
                    char formatted[BUFFER_SIZE];
                    if (pinned == 1) {
                        snprintf(formatted, sizeof(formatted), "[%s] %s -> %s: %s [PINNED]", 
                                timestamp, sender, msg_recipient, content);
                    } else {
                        snprintf(formatted, sizeof(formatted), "[%s] %s -> %s: %s", 
                                timestamp, sender, msg_recipient, content);
                    }
                    results[*result_count] = (char*)malloc(strlen(formatted) + 1);
                    strcpy(results[*result_count], formatted);
                    (*result_count)++;
                }
            }
        }
        
        free(line_copy);
    }
    
    fclose(file);
    unlock_messages_mutex();
    return results;
}

int main() {
    #ifdef _WIN32
    // Ensure server console can print and read UTF-8 logs / messages
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    #endif

    socket_t server_socket;
    if (init_server(&server_socket) < 0) {
        return 1;
    }

    printf("Waiting for clients...\n");

    while (1) {
        struct sockaddr_in client_addr;
        #ifdef _WIN32
        int addr_len = sizeof(client_addr);
        #else
        socklen_t addr_len = sizeof(client_addr);
        #endif
        
        socket_t client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            continue;
        }

        ClientThreadData* data = (ClientThreadData*)malloc(sizeof(ClientThreadData));
        data->client_socket = client_socket;
        data->client_addr = client_addr;
        data->server_state = &server_state;
        data->user = NULL;

        #ifdef _WIN32
        HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_client, data, 0, NULL);
        if (thread == NULL) {
            close_socket(client_socket);
            free(data);
        }
        #else
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, data) != 0) {
            close_socket(client_socket);
            free(data);
        }
        pthread_detach(thread);
        #endif
    }

    close_socket(server_socket);
    #ifdef _WIN32
    WSACleanup();
    #endif
    return 0;
}


