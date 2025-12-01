// Automated test for Task 2.1: Messages.txt Format with Pin Support
#include "../include/common.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

socket_t test_socket;

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
    
    printf("=== Test Task 2.1: Messages.txt Format ===\n");
    
    socket_t socket;
    if (init_test_client(&socket, server_ip) < 0) {
        printf("Failed to connect\n");
        return 1;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register user1
    strncpy(msg.sender, "testuser1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket, &msg);
    printf("Registered: testuser1\n");
    
    // Register user2
    strncpy(msg.sender, "testuser2", MAX_USERNAME - 1);
    strncpy(msg.content, "pass2", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket, &msg);
    printf("Registered: testuser2\n");
    
    // Login user1
    strncpy(msg.sender, "testuser1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket, &msg);
    printf("Logged in: testuser1\n");
    
    // Add friend
    strncpy(msg.sender, "testuser1", MAX_USERNAME - 1);
    strncpy(msg.recipient, "testuser2", MAX_USERNAME - 1);
    msg.cmd = CMD_ADD_FRIEND;
    send_and_wait(socket, &msg);
    printf("Added friend: testuser2\n");
    
    // Send message (user2 is offline, so DELIVERED should be 0)
    strncpy(msg.sender, "testuser1", MAX_USERNAME - 1);
    strncpy(msg.recipient, "testuser2", MAX_USERNAME - 1);
    strncpy(msg.content, "Test message 1", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    msg.is_pinned = false;
    send_and_wait(socket, &msg);
    printf("Sent message (user2 offline, should have DELIVERED=0)\n");
    
    // Send pinned message
    strncpy(msg.content, "Test pinned message", MAX_CONTENT - 1);
    msg.is_pinned = true;
    send_and_wait(socket, &msg);
    printf("Sent pinned message (should have PINNED=1)\n");
    
    close_socket(socket);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    printf("\nTest completed. Check messages.txt for format verification.\n");
    return 0;
}




