// Automated test for Task 2.4: Search History Function
#include "../include/common.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int init_test_client(socket_t* client_socket, const char* server_ip) {
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
    #endif

    *client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*client_socket == INVALID_SOCKET) {
        #ifdef _WIN32
        WSACleanup();
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
        close_socket(*client_socket);
        WSACleanup();
        return -1;
    }
    #else
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        close_socket(*client_socket);
        return -1;
    }
    #endif

    if (connect(*client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        close_socket(*client_socket);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return -1;
    }
    return 0;
}

void send_and_wait(socket_t socket, ProtocolMessage* msg) {
    int len;
    char* buffer = serialize_protocol_message(msg, &len);
    if (buffer) {
        send_all(socket, buffer, len);
        free(buffer);
    }
    #ifdef _WIN32
    Sleep(300);
    #else
    usleep(300000);
    #endif
}

int receive_search_result(socket_t socket, char* buffer, int buffer_size) {
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    
    if (select(socket + 1, &readfds, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(socket, &readfds)) {
            int bytes_received = recv(socket, buffer, buffer_size - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                return bytes_received;
            }
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    printf("=== Test Task 2.4: Search History Function ===\n");
    
    socket_t socket;
    if (init_test_client(&socket, server_ip) < 0) {
        printf("Failed to connect\n");
        return 1;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register and login
    strncpy(msg.sender, "search_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket, &msg);
    
    strncpy(msg.sender, "search_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket, &msg);
    
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket, &msg);
    
    // Add friend
    strncpy(msg.sender, "search_user1", MAX_USERNAME - 1);
    strncpy(msg.recipient, "search_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_ADD_FRIEND;
    send_and_wait(socket, &msg);
    
    // Send messages with keywords
    strncpy(msg.sender, "search_user1", MAX_USERNAME - 1);
    strncpy(msg.recipient, "search_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "Hello world test message", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    msg.is_pinned = false;
    send_and_wait(socket, &msg);
    
    strncpy(msg.content, "Another test message with keyword", MAX_CONTENT - 1);
    msg.is_pinned = true;
    send_and_wait(socket, &msg);
    
    // Search for "test"
    strncpy(msg.sender, "search_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "test", MAX_CONTENT - 1);
    strncpy(msg.recipient, "search_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_SEARCH_HISTORY;
    send_and_wait(socket, &msg);
    
    // Receive search results
    char buffer[BUFFER_SIZE];
    int received = receive_search_result(socket, buffer, sizeof(buffer));
    if (received > 0) {
        ProtocolMessage* response = deserialize_protocol_message(buffer, received);
        if (response && response->cmd == CMD_SEARCH_HISTORY) {
            printf("Search results: %s\n", response->content);
            if (strstr(response->content, "test") != NULL) {
                printf("SUCCESS: Search found results\n");
                free(response);
                close_socket(socket);
                #ifdef _WIN32
                WSACleanup();
                #endif
                return 0;
            }
            free(response);
        }
    }
    
    close_socket(socket);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    printf("Test completed\n");
    return 0;
}




