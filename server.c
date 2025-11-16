#include "server.h"  // Includes common.h which has socket libraries
#include <ctype.h>

// Socket libraries are included via common.h:
// Windows: winsock2.h, ws2tcpip.h, windows.h
// Linux: sys/socket.h, netinet/in.h, arpa/inet.h, sys/types.h, netdb.h

ServerState server_state;

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
        int sent = send(socket, buffer, len, 0);
        if (sent == SOCKET_ERROR) {
            #ifdef _WIN32
            printf("Send failed: %d\n", WSAGetLastError());
            #else
            printf("Send failed: %s\n", strerror(errno));
            #endif
        }
        free(buffer);
    }
}



