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
    
    // Append \n at the end for stream delimiter
    snprintf(buffer, BUFFER_SIZE, 
             "CMD:%d|SENDER:%s|RECIPIENT:%s|CONTENT:%s|EXTRA:%s|TYPE:%d|PINNED:%d|\n",
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
    
    // Trim newline at the end if present (from recv_line)
    int str_len = strlen(buffer);
    if (str_len > 0 && buffer[str_len - 1] == '\n') {
        buffer[str_len - 1] = '\0';
    }

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
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

// Ensure all data is sent
int send_all(socket_t socket, const char* data, int len) {
    int total_sent = 0;
    while (total_sent < len) {
        int sent = send(socket, data + total_sent, len - total_sent, 0);
        if (sent == SOCKET_ERROR) {
            return -1;
        }
        total_sent += sent;
    }
    return total_sent;
}

// Read until newline or buffer full (blocking)
// Returns bytes read or -1 on error/close
int recv_line(socket_t socket, char* buffer, int size) {
    int total_read = 0;
    char c;
    while (total_read < size - 1) {
        int received = recv(socket, &c, 1, 0);
        if (received <= 0) {
            return -1; // Error or closed
        }
        
        buffer[total_read++] = c;
        if (c == '\n') {
            break;
        }
    }
    buffer[total_read] = '\0';
    return total_read;
}

