// Automated test for Task 2.5: Block User Logic
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

int receive_response(socket_t socket, char* buffer, int buffer_size) {
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
    
    printf("=== Test Task 2.5: Block User Logic ===\n");
    
    // Test: A blocks B, then B tries to send message to A (should be rejected)
    printf("\n[Test] UserA blocks UserB, then UserB tries to send message...\n");
    
    // Client 1: UserA
    socket_t socket1;
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("Failed to connect\n");
        return 1;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register users
    strncpy(msg.sender, "block_userA", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "block_userB", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    // Login UserA
    strncpy(msg.sender, "block_userA", MAX_USERNAME - 1);
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket1, &msg);
    
    // UserA blocks UserB
    strncpy(msg.sender, "block_userA", MAX_USERNAME - 1);
    strncpy(msg.recipient, "block_userB", MAX_USERNAME - 1);
    msg.cmd = CMD_BLOCK_USER;
    send_and_wait(socket1, &msg);
    printf("UserA blocked UserB\n");
    
    close_socket(socket1);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // Client 2: UserB tries to send message to UserA (should be rejected)
    socket_t socket2;
    if (init_test_client(&socket2, server_ip) < 0) {
        printf("Failed to connect\n");
        return 1;
    }
    
    memset(&msg, 0, sizeof(ProtocolMessage));
    strncpy(msg.sender, "block_userB", MAX_USERNAME - 1);
    strncpy(msg.content, "pass2", MAX_CONTENT - 1);
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket2, &msg);
    
    // Wait for login response and verify
    char login_buffer[BUFFER_SIZE];
    int login_received = receive_response(socket2, login_buffer, sizeof(login_buffer));
    if (login_received > 0) {
        ProtocolMessage* login_response = deserialize_protocol_message(login_buffer, login_received);
        if (login_response) {
            printf("Login response: cmd=%d, content=%s\n", login_response->cmd, login_response->content);
            if (login_response->cmd != CMD_SUCCESS) {
                printf("FAILED: Login failed\n");
                free(login_response);
                close_socket(socket2);
                #ifdef _WIN32
                WSACleanup();
                #endif
                return 1;
            }
            free(login_response);
        }
    }
    
    #ifdef _WIN32
    Sleep(200);
    #else
    usleep(200000);
    #endif
    
    // UserB tries to send message to UserA
    memset(&msg, 0, sizeof(ProtocolMessage));
    strncpy(msg.sender, "block_userB", MAX_USERNAME - 1);
    strncpy(msg.recipient, "block_userA", MAX_USERNAME - 1);
    strncpy(msg.content, "Test message", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    send_and_wait(socket2, &msg);
    
    // Check response - try multiple times
    char buffer[BUFFER_SIZE];
    for (int i = 0; i < 5; i++) {
        int received = receive_response(socket2, buffer, sizeof(buffer));
        if (received > 0) {
            ProtocolMessage* response = deserialize_protocol_message(buffer, received);
            if (response) {
                printf("Received response: cmd=%d, content=%s\n", response->cmd, response->content);
                if (response->cmd == CMD_ERROR && strstr(response->content, "blocked") != NULL) {
                    printf("SUCCESS: Message correctly rejected - %s\n", response->content);
                    free(response);
                    close_socket(socket2);
                    #ifdef _WIN32
                    WSACleanup();
                    #endif
                    return 0;
                }
                free(response);
            }
        }
        #ifdef _WIN32
        Sleep(200);
        #else
        usleep(200000);
        #endif
    }
    
    close_socket(socket2);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    printf("Test completed\n");
    return 1;
}

