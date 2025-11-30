#include "common.h"

// Log activity to file
void log_activity(const char* username, const char* action, const char* details) {
    FILE* log_file = fopen("activity.log", "a");
    if (log_file) {
        time_t now = time(NULL);
        char* time_str = get_timestamp_string(now);
        fprintf(log_file, "[%s] User: %s | Action: %s | Details: %s\n", 
                time_str, username, action, details);
        fclose(log_file);
        free(time_str);
    }
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

