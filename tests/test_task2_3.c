// Automated test for Task 2.3: Thread Safety and Pin Support
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

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    printf("=== Test Task 2.3: Thread Safety and Pin Support ===\n");
    
    // Test 1: Send pinned message
    printf("\n[Test 1] Sending pinned message...\n");
    socket_t socket1;
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("Failed to connect\n");
        return 1;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register and login
    strncpy(msg.sender, "pin_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "pin_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket1, &msg);
    
    // Add friend
    strncpy(msg.sender, "pin_user1", MAX_USERNAME - 1);
    strncpy(msg.recipient, "pin_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_ADD_FRIEND;
    send_and_wait(socket1, &msg);
    
    // Send pinned message
    strncpy(msg.sender, "pin_user1", MAX_USERNAME - 1);
    strncpy(msg.recipient, "pin_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "This is a pinned message", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    msg.is_pinned = true;  // PINNED = 1
    send_and_wait(socket1, &msg);
    printf("Sent pinned message (PINNED=1)\n");
    
    // Send unpinned message
    strncpy(msg.content, "This is a normal message", MAX_CONTENT - 1);
    msg.is_pinned = false;  // PINNED = 0
    send_and_wait(socket1, &msg);
    printf("Sent normal message (PINNED=0)\n");
    
    close_socket(socket1);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    printf("\nTest completed. Check messages.txt for PINNED flags.\n");
    return 0;
}




