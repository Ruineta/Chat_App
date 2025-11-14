#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    // Windows Socket API libraries
    #include <winsock2.h>      // Main Windows socket library
    #include <ws2tcpip.h>      // TCP/IP definitions for Windows
    #include <windows.h>       // Windows API
    #define close_socket closesocket
    #ifndef INVALID_SOCKET
    #define INVALID_SOCKET  (SOCKET)(~0)
    #endif
    #ifndef SOCKET_ERROR
    #define SOCKET_ERROR (-1)
    #endif
    typedef SOCKET socket_t;
#else
    // Linux/Unix Socket API libraries
    #include <sys/socket.h>   // Main socket library
    #include <netinet/in.h>    // Internet address family
    #include <arpa/inet.h>     // IP address conversion functions
    #include <unistd.h>        // POSIX operating system API
    #include <pthread.h>       // POSIX threads
    #include <sys/types.h>     // System data types
    #include <netdb.h>         // Network database operations
    typedef int socket_t;
    #define close_socket close
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

#define MAX_USERNAME 50
#define MAX_MESSAGE 1024
#define MAX_CONTENT 2048
#define MAX_GROUP_NAME 100
#define MAX_GROUP_ID 50
#define MAX_FRIENDS 100
#define MAX_GROUPS 50
#define MAX_MEMBERS 100
#define PORT 8080
#define BUFFER_SIZE 4096

// Message types
typedef enum {
    MSG_TEXT = 0,
    MSG_EMOJI = 1,
    MSG_SYSTEM = 2
} MessageType;

// Command types
typedef enum {
    CMD_LOGIN = 0,
    CMD_REGISTER = 1,
    CMD_LOGOUT = 2,
    CMD_GET_FRIENDS = 3,
    CMD_ADD_FRIEND = 18,
    CMD_SEND_MESSAGE = 4,
    CMD_RECEIVE_MESSAGE = 5,
    CMD_DISCONNECT = 6,
    CMD_CREATE_GROUP = 7,
    CMD_ADD_TO_GROUP = 8,
    CMD_REMOVE_FROM_GROUP = 9,
    CMD_LEAVE_GROUP = 10,
    CMD_GROUP_MESSAGE = 11,
    CMD_SEARCH_HISTORY = 12,
    CMD_SET_GROUP_NAME = 13,
    CMD_BLOCK_USER = 14,
    CMD_UNBLOCK_USER = 15,
    CMD_PIN_MESSAGE = 16,
    CMD_GET_PINNED = 17,
    CMD_ERROR = 99,
    CMD_SUCCESS = 100
} CommandType;

// Message structure
typedef struct {
    char id[100];
    char sender[MAX_USERNAME];
    char content[MAX_CONTENT];
    MessageType type;
    time_t timestamp;
    bool is_pinned;
} Message;

// User structure
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_USERNAME];
    bool is_online;
    socket_t socket;
    char blocked_users[MAX_FRIENDS][MAX_USERNAME];
    int blocked_count;
    char friends[MAX_FRIENDS][MAX_USERNAME];
    int friend_count;
} User;

// Group structure
typedef struct {
    char group_id[MAX_GROUP_ID];
    char name[MAX_GROUP_NAME];
    char creator[MAX_USERNAME];
    char members[MAX_MEMBERS][MAX_USERNAME];
    int member_count;
    char admins[MAX_MEMBERS][MAX_USERNAME];
    int admin_count;
    Message messages[1000];
    int message_count;
    time_t created_at;
} Group;

// Protocol message structure
typedef struct {
    CommandType cmd;
    char sender[MAX_USERNAME];
    char recipient[MAX_USERNAME];  // Can be user or group_id
    char content[MAX_CONTENT];
    char extra_data[500];  // For additional info like group_id, search keyword, etc.
    MessageType msg_type;
    bool is_pinned;
} ProtocolMessage;

// Function declarations
void log_activity(const char* username, const char* action, const char* details);
char* serialize_protocol_message(ProtocolMessage* msg, int* len);
ProtocolMessage* deserialize_protocol_message(char* buffer, int len);
char* get_timestamp_string(time_t t);
void trim_newline(char* str);

#endif // COMMON_H

