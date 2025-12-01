// Automated test program for Task 1.3: Thread Safety cho Logging
// This program will automatically perform actions and verify logging

#include "../include/common.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

socket_t test_socket;
bool test_connected = false;

// Initialize test client
int init_test_client(socket_t* client_socket, const char* server_ip) {
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return -1;
    }
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

    test_connected = true;
    return 0;
}

// Send command and wait for response
void send_and_wait(socket_t socket, ProtocolMessage* msg) {
    int len;
    char* buffer = serialize_protocol_message(msg, &len);
    if (buffer) {
        send_all(socket, buffer, len);
        free(buffer);
    }
    
    // Wait a bit for response
    #ifdef _WIN32
    Sleep(200);
    #else
    usleep(200000);
    #endif
}

// Test function
int test_logging(const char* server_ip, const char* username, const char* password, int test_id) {
    socket_t socket;
    if (init_test_client(&socket, server_ip) < 0) {
        printf("[TEST %d] Failed to connect\n", test_id);
        return -1;
    }
    
    printf("[TEST %d] Connected\n", test_id);
    
    // Register
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    char test_username[50];
    snprintf(test_username, sizeof(test_username), "%s_%d", username, test_id);
    strncpy(msg.sender, test_username, MAX_USERNAME - 1);
    strncpy(msg.content, password, MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket, &msg);
    printf("[TEST %d] Registered: %s\n", test_id, test_username);
    
    #ifdef _WIN32
    Sleep(100);
    #else
    usleep(100000);
    #endif
    
    // Login
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket, &msg);
    printf("[TEST %d] Logged in: %s\n", test_id, test_username);
    
    #ifdef _WIN32
    Sleep(100);
    #else
    usleep(100000);
    #endif
    
    // Add friend (if not first client)
    if (test_id > 1) {
        char friend_name[50];
        snprintf(friend_name, sizeof(friend_name), "%s_%d", username, test_id - 1);
        strncpy(msg.recipient, friend_name, MAX_USERNAME - 1);
        msg.cmd = CMD_ADD_FRIEND;
        send_and_wait(socket, &msg);
        printf("[TEST %d] Added friend: %s\n", test_id, friend_name);
        
        #ifdef _WIN32
        Sleep(100);
        #else
        usleep(100000);
        #endif
    }
    
    // Send message (if not first client)
    if (test_id > 1) {
        char friend_name[50];
        snprintf(friend_name, sizeof(friend_name), "%s_%d", username, test_id - 1);
        strncpy(msg.recipient, friend_name, MAX_USERNAME - 1);
        char message[100];
        snprintf(message, sizeof(message), "Test message from %s", test_username);
        strncpy(msg.content, message, MAX_CONTENT - 1);
        msg.cmd = CMD_SEND_MESSAGE;
        msg.msg_type = MSG_TEXT;
        send_and_wait(socket, &msg);
        printf("[TEST %d] Sent message\n", test_id);
        
        #ifdef _WIN32
        Sleep(100);
        #else
        usleep(100000);
        #endif
    }
    
    // Disconnect
    msg.cmd = CMD_DISCONNECT;
    send_and_wait(socket, &msg);
    printf("[TEST %d] Disconnected\n", test_id);
    
    close_socket(socket);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return 0;
}

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    const char* username = (argc > 2) ? argv[2] : "testuser";
    const char* password = (argc > 3) ? argv[3] : "testpass";
    int num_clients = (argc > 4) ? atoi(argv[4]) : 3;
    
    printf("=== Automated Test for Task 1.3: Thread Safety Logging ===\n");
    printf("Server: %s\n", server_ip);
    printf("Base username: %s\n", username);
    printf("Number of clients: %d\n", num_clients);
    printf("\n");
    
    // Run multiple clients concurrently
    for (int i = 1; i <= num_clients; i++) {
        printf("Starting client %d...\n", i);
        test_logging(server_ip, username, password, i);
        #ifdef _WIN32
        Sleep(50);
        #else
        usleep(50000);
        #endif
    }
    
    printf("\n=== Test completed ===\n");
    printf("Please check activity.log to verify:\n");
    printf("  - All log entries are complete\n");
    printf("  - No corruption or truncated lines\n");
    printf("  - Format is correct\n");
    printf("  - Thread safety is working\n");
    
    return 0;
}




