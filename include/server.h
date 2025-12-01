#ifndef SERVER_H
#define SERVER_H

#include "common.h"

// Server state
typedef struct {
    User users[1000];
    int user_count;
    Group groups[100];
    int group_count;
    Message conversations[5000];  // Store all 1-1 messages
    int conversation_count;
    #ifdef _WIN32
    CRITICAL_SECTION mutex;
    #else
    pthread_mutex_t mutex;
    #endif
} ServerState;

// Client handler thread data
typedef struct {
    socket_t client_socket;
    struct sockaddr_in client_addr;
    ServerState* server_state;
    User* user;
} ClientThreadData;

// Function declarations
int init_server(socket_t* server_socket);
#ifdef _WIN32
DWORD WINAPI handle_client(LPVOID arg);
#else
void* handle_client(void* arg);
#endif
User* find_user(ServerState* state, const char* username);
Group* find_group(ServerState* state, const char* group_id);
void add_user(ServerState* state, const char* username, const char* password);
void send_response(socket_t socket, CommandType cmd, const char* content);
void broadcast_to_friends(ServerState* state, const char* username, const char* message);
void notify_recent_chatters(ServerState* state, const char* disconnected_username);
void save_message_to_file(const char* sender, const char* recipient, const char* content, bool is_group, bool recipient_online, bool is_pinned);
void load_and_send_offline_messages(socket_t socket, const char* username);
char** search_messages(const char* keyword, const char* username, const char* recipient, int* result_count);
void update_message_pin_status_in_file(const char* sender, const char* content, const char* timestamp, int is_pinned);
// Account persistence
int load_accounts(const char* filename);
int save_account(const char* filename, const char* username, const char* password);

#endif // SERVER_H

