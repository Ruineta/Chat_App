#include "server.h"  // Includes common.h which has socket libraries
#include <ctype.h>
#include "common.h"

// Socket libraries are included via common.h:
// Windows: winsock2.h, ws2tcpip.h, windows.h
// Linux: sys/socket.h, netinet/in.h, arpa/inet.h, sys/types.h, netdb.h

ServerState server_state;

#define ACCOUNT_FILE "account.txt"
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

// Save all friends and requests to friends.txt
// Format: username|friend1,friend2,...|req1,req2,...
void save_friends_state(ServerState* state) {
    FILE* file = fopen("friends.txt", "w");
    if (!file) return;

    for (int i = 0; i < state->user_count; i++) {
        User* u = &state->users[i];
        fprintf(file, "%s|", u->username);
        
        // Write friends
        for (int j = 0; j < u->friend_count; j++) {
            fprintf(file, "%s%s", u->friends[j], (j < u->friend_count - 1) ? "," : "");
        }
        fprintf(file, "|");
        
        // Write requests
        for (int j = 0; j < u->request_count; j++) {
            fprintf(file, "%s%s", u->friend_requests[j], (j < u->request_count - 1) ? "," : "");
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

// Load friends state
void load_friends_state(ServerState* state) {
    FILE* file = fopen("friends.txt", "r");
    if (!file) return;

    char line[BUFFER_SIZE];
    // Re-implementation for robustness:
    // rewind(file); // No need to rewind if we start valid here
    while (fgets(line, sizeof(line), file)) {
        trim_newline(line);
        char* p = line;
        
        // Extract username
        char* pipe1 = strchr(p, '|');
        if (!pipe1) continue;
        *pipe1 = '\0';
        char* username = p;
        p = pipe1 + 1;
        
        User* u = find_user(state, username);
        if (!u) continue;

        // Extract friends
        char* pipe2 = strchr(p, '|');
        if (!pipe2) continue;
        *pipe2 = '\0';
        char* friends_part = p;
        char* requests_part = pipe2 + 1;
        
        // Parse friends
        if (strlen(friends_part) > 0) {
            char* ctx;
            char* f = strtok(friends_part, ",");
            while (f) {
                if (u->friend_count < MAX_FRIENDS) {
                    strncpy(u->friends[u->friend_count++], f, MAX_USERNAME - 1);
                }
                f = strtok(NULL, ",");
            }
        }

        // Parse requests
        if (strlen(requests_part) > 0) {
            char* req = strtok(requests_part, ",");
            while (req) {
                 if (u->request_count < MAX_FRIENDS)
                    strncpy(u->friend_requests[u->request_count++], req, MAX_USERNAME - 1);
                req = strtok(NULL, ",");
            }
        }
    }
    fclose(file);
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

    // Load accounts into server_state from persistence file
    int loaded = load_accounts(ACCOUNT_FILE);
    if (loaded < 0) {
        printf("Warning: failed to load accounts from %s\n", ACCOUNT_FILE);
    } else if (loaded > 0) {
        printf("Loaded %d accounts from %s\n", loaded, ACCOUNT_FILE);
    }
    
    load_friends_state(&server_state);
    printf("Loaded friend data\n");

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
    new_user->last_seen = 0;
    new_user->blocked_count = 0;
    new_user->friend_count = 0;
    new_user->request_count = 0;
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
        if (send_all(socket, buffer, len) < 0) {
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
    if (!are_friends(user1, user2)) {
        strncpy(user1->friends[user1->friend_count++], user2->username, MAX_USERNAME - 1);
    }
    if (!are_friends(user2, user1)) {
        strncpy(user2->friends[user2->friend_count++], user1->username, MAX_USERNAME - 1);
    }
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
        // Use recv_line to read a full message ending with \n
        int bytes_received = recv_line(client_socket, buffer, BUFFER_SIZE);
        
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
                if (user) {
                    if (user->is_online) {
                        send_response(client_socket, CMD_ERROR, "User already logged in on another device");
                        break;
                    }
                    
                    if (strcmp(user->password, msg->content) == 0) {
                        user->is_online = true;
                        user->socket = client_socket;
                        current_user = user;
                        send_response(client_socket, CMD_SUCCESS, "Login successful");
                        log_activity(msg->sender, "LOGIN", "User logged in");
                        
                        // Send offline messages
                        // Implementation would load from file
                    } else {
                        send_response(client_socket, CMD_ERROR, "Invalid credentials");
                    }
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
                
                char friend_list[BUFFER_SIZE] = "Friends: ";
                if (current_user->friend_count == 0) {
                    strcat(friend_list, "(none)");
                } else {
                    for (int i = 0; i < current_user->friend_count; i++) {
                        User* f = find_user(state, current_user->friends[i]);
                        if (f) {
                            char info[200];
                            if (f->is_online) {
                                snprintf(info, sizeof(info), "%s [ONLINE]; ", f->username);
                            } else {
                                char* last_seen_str = get_timestamp_string(f->last_seen);
                                snprintf(info, sizeof(info), "%s [OFFLINE] (Last seen: %s); ", f->username, last_seen_str);
                                free(last_seen_str);
                            }
                            strcat(friend_list, info);
                        }
                    }
                }
                send_response(client_socket, CMD_GET_FRIENDS, friend_list);
                break;
            }
            
            case CMD_ADD_FRIEND: {
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
                save_friends_state(state); // Persist
                send_response(client_socket, CMD_SUCCESS, "Friend added");
                log_activity(current_user->username, "ADD_FRIEND", msg->recipient);
                break;
            }

            case CMD_FRIEND_REQUEST: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in"); break; 
                }
                if (strcmp(current_user->username, msg->recipient) == 0) {
                    send_response(client_socket, CMD_ERROR, "Cannot request yourself"); break;
                }
                
                User* target = find_user(state, msg->recipient);
                if (!target) {
                    send_response(client_socket, CMD_ERROR, "User not found"); break;
                }
                
                if (are_friends(current_user, target)) {
                    send_response(client_socket, CMD_ERROR, "Already friends"); break;
                }
                
                // Check if already requested
                bool already_requested = false;
                for (int i=0; i < target->request_count; i++) {
                    if (strcmp(target->friend_requests[i], current_user->username) == 0) {
                        already_requested = true; break;
                    }
                }
                if (already_requested) {
                    send_response(client_socket, CMD_ERROR, "Request already sent"); break;
                }
                
                strncpy(target->friend_requests[target->request_count++], current_user->username, MAX_USERNAME - 1);
                save_friends_state(state);
                send_response(client_socket, CMD_SUCCESS, "Friend request sent");
                
                // Notify target if online
                if (target->is_online && target->socket != INVALID_SOCKET) {
                   ProtocolMessage notify_msg;
                   memset(&notify_msg, 0, sizeof(ProtocolMessage));
                   notify_msg.cmd = CMD_RECEIVE_MESSAGE; // Use generic receive or create a notification cmd? 
                   // Let's use CMD_RECEIVE_MESSAGE with MSG_SYSTEM type for simplicity
                   notify_msg.msg_type = MSG_SYSTEM;
                   strcpy(notify_msg.sender, "SYSTEM");
                   strcpy(notify_msg.recipient, target->username);
                   snprintf(notify_msg.content, sizeof(notify_msg.content), "You have a new friend request from %s", current_user->username);
                   
                   int len;
                   char* data = serialize_protocol_message(&notify_msg, &len);
                   send_all(target->socket, data, len);
                   free(data);
                }
                break;
            }

            case CMD_FRIEND_ACCEPT: {
                 if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in"); break; 
                }
                char* requester_name = msg->recipient; // Reusing recipient field for target username
                bool found_req = false;
                for (int i=0; i < current_user->request_count; i++) {
                    if (strcmp(current_user->friend_requests[i], requester_name) == 0) {
                        // Remove request
                        for (int j=i; j < current_user->request_count - 1; j++) {
                            strcpy(current_user->friend_requests[j], current_user->friend_requests[j+1]);
                        }
                        current_user->request_count--;
                        found_req = true;
                        break;
                    }
                }
                
                if (!found_req) {
                    send_response(client_socket, CMD_ERROR, "Request not found"); break;
                }
                
                User* requester = find_user(state, requester_name);
                if (requester) {
                    add_friend(current_user, requester);
                    save_friends_state(state);
                    send_response(client_socket, CMD_SUCCESS, "Friend request accepted");

                    // Notify requester if online
                    if (requester->is_online && requester->socket != INVALID_SOCKET) {
                       ProtocolMessage notify_msg;
                       memset(&notify_msg, 0, sizeof(ProtocolMessage));
                       notify_msg.cmd = CMD_RECEIVE_MESSAGE; 
                       notify_msg.msg_type = MSG_SYSTEM;
                       strcpy(notify_msg.sender, "SYSTEM");
                       strcpy(notify_msg.recipient, requester->username);
                       snprintf(notify_msg.content, sizeof(notify_msg.content), "%s accepted your friend request", current_user->username);
                       
                       int len;
                       char* data = serialize_protocol_message(&notify_msg, &len);
                       send_all(requester->socket, data, len);
                       free(data);
                    }
                }
                break;
            }

            case CMD_FRIEND_REJECT: {
                 if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in"); break; 
                }
                char* requester_name = msg->recipient;
                bool found_req = false;
                for (int i=0; i < current_user->request_count; i++) {
                    if (strcmp(current_user->friend_requests[i], requester_name) == 0) {
                        // Remove request
                        for (int j=i; j < current_user->request_count - 1; j++) {
                            strcpy(current_user->friend_requests[j], current_user->friend_requests[j+1]);
                        }
                        current_user->request_count--;
                        found_req = true;
                        break;
                    }
                }
                if (found_req) {
                    save_friends_state(state);
                    send_response(client_socket, CMD_SUCCESS, "Friend request rejected");
                } else {
                    send_response(client_socket, CMD_ERROR, "Request not found");
                }
                break;
            }

            case CMD_GET_REQUESTS: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in"); break;
                }
                char req_list[BUFFER_SIZE] = "Friend Requests: ";
                if (current_user->request_count == 0) {
                    strcat(req_list, "(none)");
                } else {
                    for (int i = 0; i < current_user->request_count; i++) {
                        strcat(req_list, current_user->friend_requests[i]);
                        strcat(req_list, " ");
                    }
                }
                send_response(client_socket, CMD_GET_REQUESTS, req_list);
                break;
            }
            
            case CMD_REMOVE_FRIEND: { // UNFRIEND
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in"); break;
                }
                User* friend_user = find_user(state, msg->recipient);
                
                bool removed = false;
                // Remove forward
                for (int i=0; i < current_user->friend_count; i++) {
                    if (strcmp(current_user->friends[i], msg->recipient) == 0) {
                         for (int j=i; j < current_user->friend_count - 1; j++) {
                            strcpy(current_user->friends[j], current_user->friends[j+1]);
                        }
                        current_user->friend_count--;
                        removed = true;
                        break;
                    }
                }
                
                // Remove backward
                if (friend_user) {
                    for (int i=0; i < friend_user->friend_count; i++) {
                        if (strcmp(friend_user->friends[i], current_user->username) == 0) {
                             for (int j=i; j < friend_user->friend_count - 1; j++) {
                                strcpy(friend_user->friends[j], friend_user->friends[j+1]);
                            }
                            friend_user->friend_count--;
                            break;
                        }
                    }
                }
                
                if (removed) {
                    save_friends_state(state);
                    send_response(client_socket, CMD_SUCCESS, "Friend removed");
                } else {
                    send_response(client_socket, CMD_ERROR, "Not friends");
                }
                break;
            }
            
            case CMD_SEND_MESSAGE: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }
                
                User* recipient = find_user(state, msg->recipient);
                if (!recipient) {
                    send_response(client_socket, CMD_ERROR, "Recipient not found");
                    break;
                }
                
                if (is_blocked(current_user, msg->recipient) || is_blocked(recipient, current_user->username)) {
                    send_response(client_socket, CMD_ERROR, "User is blocked");
                    break;
                }
                
                // Create message
                Message message;
                time_t now = time(NULL);
                snprintf(message.id, sizeof(message.id), "%s_%lld", current_user->username, (long long)now);
                strncpy(message.sender, current_user->username, MAX_USERNAME - 1);
                strncpy(message.content, msg->content, MAX_CONTENT - 1);
                message.type = msg->msg_type;
                message.timestamp = time(NULL);
                message.is_pinned = msg->is_pinned;
                
                // Save message
                save_message_to_file(current_user->username, msg->recipient, msg->content, false);
                
                // Send to recipient if online
                if (recipient->is_online && recipient->socket != INVALID_SOCKET) {
                    ProtocolMessage response;
                    memset(&response, 0, sizeof(ProtocolMessage));
                    response.cmd = CMD_RECEIVE_MESSAGE;
                    strncpy(response.sender, current_user->username, MAX_USERNAME - 1);
                    strncpy(response.content, msg->content, MAX_CONTENT - 1);
                    response.msg_type = msg->msg_type;
                    
                    int len;
                    char* resp_buffer = serialize_protocol_message(&response, &len);
                    if (send_all(recipient->socket, resp_buffer, len) < 0) {
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
            
            case CMD_LOGOUT: {
                if (!current_user) {
                    send_response(client_socket, CMD_ERROR, "Not logged in");
                    break;
                }

                /* mark user offline but keep connection open */
                current_user->is_online = false;
                current_user->socket = INVALID_SOCKET;
                current_user->last_seen = time(NULL);
                send_response(client_socket, CMD_SUCCESS, "Logged out");
                log_activity(current_user->username, "LOGOUT", "User logged out");

                /* notify friends user went offline */
                broadcast_to_friends(state, current_user->username, "User logged out");

                /* forget current_user for this connection so new login may happen */
                current_user = NULL;
                break;
            }

            case CMD_DISCONNECT: {
                if (current_user) {
                    current_user->is_online = false;
                    current_user->socket = INVALID_SOCKET;
                    current_user->last_seen = time(NULL);
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
                snprintf(group_id, sizeof(group_id), "GROUP_%s_%lld", current_user->username, (long long)now);
                
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
                        // Shift remaining members
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
                snprintf(group_msg->id, sizeof(group_msg->id), "%s_%lld", current_user->username, (long long)now);
                strncpy(group_msg->sender, current_user->username, MAX_USERNAME - 1);
                strncpy(group_msg->content, msg->content, MAX_CONTENT - 1);
                group_msg->type = msg->msg_type;
                group_msg->timestamp = time(NULL);
                group_msg->is_pinned = msg->is_pinned;
                
                save_message_to_file(current_user->username, msg->recipient, msg->content, true);
                
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
                        if (send_all(member->socket, resp_buffer, len) < 0) {
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
                                send_response(client_socket, CMD_SUCCESS, "Message pinned");
                                log_activity(current_user->username, "PIN_MESSAGE", msg->extra_data);
                                break;
                            }
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
            
            case CMD_CHECK_STATUS: {
                // Return status and last seen of a user
                User* target = find_user(state, msg->recipient);
                if (target) {
                    char status_info[BUFFER_SIZE];
                    if (target->is_online) {
                        snprintf(status_info, sizeof(status_info), "User %s is ONLINE", target->username);
                    } else {
                        char* time_str = get_timestamp_string(target->last_seen);
                        snprintf(status_info, sizeof(status_info), "User %s is OFFLINE (Last seen: %s)", 
                                target->username, time_str);
                        free(time_str);
                    }
                    send_response(client_socket, CMD_SUCCESS, status_info);
                } else {
                    send_response(client_socket, CMD_ERROR, "User not found");
                }
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
    }
    
    close_socket(client_socket);
    free(data);
    #ifdef _WIN32
    return 0;
    #else
    return NULL;
    #endif
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
            if (send_all(friend->socket, buffer, len) < 0) {
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
void save_message_to_file(const char* sender, const char* recipient, const char* content, bool is_group) {
    FILE* file = fopen("messages.txt", "a");
    if (file) {
        time_t now = time(NULL);
        // Format: MSG_ID|TIMESTAMP|SENDER|RECIPIENT|CONTENT
        // Generating a simple MSG_ID based on timestamp for now, similar to how it's done in SEND_MESSAGE
        char msg_id[100];
        snprintf(msg_id, sizeof(msg_id), "%s_%lld", sender, (long long)now);
        
        fprintf(file, "%s|%lld|%s|%s|%s\n", 
                msg_id, (long long)now, sender, recipient, content);
        fclose(file);
    }
}

// Search messages
char** search_messages(const char* keyword, const char* username, const char* recipient, int* result_count) {
    char** results = (char**)malloc(100 * sizeof(char*));
    *result_count = 0;
    
    FILE* file = fopen("messages.txt", "r");
    if (!file) return results;
    
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file) && *result_count < 100) {
        if (strstr(line, keyword) != NULL) {
            // Check if message is relevant to this user
            if (strstr(line, username) != NULL && (strlen(recipient) == 0 || strstr(line, recipient) != NULL)) {
                results[*result_count] = (char*)malloc(strlen(line) + 1);
                strcpy(results[*result_count], line);
                (*result_count)++;
            }
        }
    }
    
    fclose(file);
    return results;
}

// Main server function
int main() {
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


