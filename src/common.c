#include "../include/common.h"

// Thread-safe logging mutex
#ifdef _WIN32
static CRITICAL_SECTION log_mutex;
static bool log_mutex_initialized = false;
#else
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// Thread-safe messages file mutex
#ifdef _WIN32
static CRITICAL_SECTION messages_mutex;
static bool messages_mutex_initialized = false;
#else
static pthread_mutex_t messages_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// Initialize log mutex
void init_log_mutex(void) {
    #ifdef _WIN32
    if (!log_mutex_initialized) {
        InitializeCriticalSection(&log_mutex);
        log_mutex_initialized = true;
    }
    #else
    // Already initialized with PTHREAD_MUTEX_INITIALIZER
    #endif
}

// Initialize messages file mutex
void init_messages_mutex(void) {
    #ifdef _WIN32
    if (!messages_mutex_initialized) {
        InitializeCriticalSection(&messages_mutex);
        messages_mutex_initialized = true;
    }
    #else
    // Already initialized with PTHREAD_MUTEX_INITIALIZER
    #endif
}

// Lock messages file mutex
void lock_messages_mutex(void) {
    #ifdef _WIN32
    EnterCriticalSection(&messages_mutex);
    #else
    pthread_mutex_lock(&messages_mutex);
    #endif
}

// Unlock messages file mutex
void unlock_messages_mutex(void) {
    #ifdef _WIN32
    LeaveCriticalSection(&messages_mutex);
    #else
    pthread_mutex_unlock(&messages_mutex);
    #endif
}

// Log activity to file (thread-safe)
void log_activity(const char* username, const char* action, const char* details) {
    // Lock mutex before file operations
    #ifdef _WIN32
    EnterCriticalSection(&log_mutex);
    #else
    pthread_mutex_lock(&log_mutex);
    #endif
    
    FILE* log_file = fopen("../logs/activity.log", "a");
    if (log_file) {
        time_t now = time(NULL);
        char* time_str = get_timestamp_string(now);
        fprintf(log_file, "[%s] User: %s | Action: %s | Details: %s\n", 
                time_str, username, action, details);
        fflush(log_file);  // Ensure data is written immediately
        fclose(log_file);
        free(time_str);
    }
    
    // Unlock mutex after file operations
    #ifdef _WIN32
    LeaveCriticalSection(&log_mutex);
    #else
    pthread_mutex_unlock(&log_mutex);
    #endif
}

// Serialize protocol message to string
char* serialize_protocol_message(ProtocolMessage* msg, int* len) {
    char* buffer = (char*)malloc(BUFFER_SIZE);
    if (!buffer) return NULL;
    
    snprintf(buffer, BUFFER_SIZE, 
             "CMD:%d|SENDER:%s|RECIPIENT:%s|CONTENT:%s|EXTRA:%s|TYPE:%d|PINNED:%d|",
             msg->cmd, msg->sender, msg->recipient, msg->content, 
             msg->extra_data, msg->msg_type, msg->is_pinned ? 1 : 0);
    
    *len = strlen(buffer);
    return buffer;
}

// Deserialize string to protocol message
ProtocolMessage* deserialize_protocol_message(char* buffer, int len) {
    (void)len;  // Suppress unused parameter warning
    ProtocolMessage* msg = (ProtocolMessage*)malloc(sizeof(ProtocolMessage));
    if (!msg) return NULL;
    
    memset(msg, 0, sizeof(ProtocolMessage));
    
    // Simple parsing
    char* token = strtok(buffer, "|");
    while (token) {
        if (strncmp(token, "CMD:", 4) == 0) {
            msg->cmd = (CommandType)atoi(token + 4);
        } else if (strncmp(token, "SENDER:", 7) == 0) {
            strncpy(msg->sender, token + 7, MAX_USERNAME - 1);
        } else if (strncmp(token, "RECIPIENT:", 10) == 0) {
            strncpy(msg->recipient, token + 10, MAX_USERNAME - 1);
        } else if (strncmp(token, "CONTENT:", 8) == 0) {
            strncpy(msg->content, token + 8, MAX_CONTENT - 1);
        } else if (strncmp(token, "EXTRA:", 6) == 0) {
            strncpy(msg->extra_data, token + 6, 499);
        } else if (strncmp(token, "TYPE:", 5) == 0) {
            msg->msg_type = (MessageType)atoi(token + 5);
        } else if (strncmp(token, "PINNED:", 7) == 0) {
            msg->is_pinned = atoi(token + 7) == 1;
        }
        token = strtok(NULL, "|");
    }
    
    return msg;
}

// Get timestamp as string
char* get_timestamp_string(time_t t) {
    char* str = (char*)malloc(50);
    struct tm* timeinfo = localtime(&t);
    strftime(str, 50, "%Y-%m-%d %H:%M:%S", timeinfo);
    return str;
}

// Remove newline from string
void trim_newline(char* str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

// Clear stdin buffer
void clear_stdin_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Send all data, handling partial sends
int send_all(socket_t sock, const char* buf, size_t len) {
    size_t total_sent = 0;
    while (total_sent < len) {
        int sent = send(sock, buf + total_sent, (int)(len - total_sent), 0);
        if (sent == SOCKET_ERROR) {
            #ifdef _WIN32
            printf("Send error: %d\n", WSAGetLastError());
            #else
            printf("Send error: %s\n", strerror(errno));
            #endif
            return -1;
        }
        if (sent == 0) {
            // Connection closed
            return -1;
        }
        total_sent += sent;
    }
    return (int)total_sent;
}

// Receive all data, handling partial receives
int recv_all(socket_t sock, char* buf, size_t len) {
    size_t total_received = 0;
    while (total_received < len) {
        int received = recv(sock, buf + total_received, (int)(len - total_received), 0);
        if (received == SOCKET_ERROR) {
            #ifdef _WIN32
            printf("Recv error: %d\n", WSAGetLastError());
            #else
            printf("Recv error: %s\n", strerror(errno));
            #endif
            return -1;
        }
        if (received == 0) {
            // Connection closed
            return (int)total_received;
        }
        total_received += received;
    }
    return (int)total_received;
}

