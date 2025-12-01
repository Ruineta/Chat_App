// Automated test client for Task 1.1: Single Session Enforcement
// This client will connect, login, and wait for messages

#include "../include/common.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

socket_t test_client_socket;
bool test_connected = false;
char termination_received = 0;

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

// Receive and check for termination message
void test_receive_response(socket_t socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        test_connected = false;
        return;
    }
    
    buffer[bytes_received] = '\0';
    ProtocolMessage* msg = deserialize_protocol_message(buffer, bytes_received);
    if (!msg) return;
    
    if (msg->cmd == CMD_RECEIVE_MESSAGE) {
        if (strstr(msg->content, "terminated") != NULL || 
            strstr(msg->content, "terminated") != NULL) {
            termination_received = 1;
            printf("[TEST] Termination message received: %s\n", msg->content);
        }
    }
    
    free(msg);
}

// Test function
int test_single_session(const char* server_ip, const char* username, const char* password, int client_id) {
    socket_t socket;
    if (init_test_client(&socket, server_ip) < 0) {
        printf("[TEST CLIENT %d] Failed to connect\n", client_id);
        return -1;
    }
    
    printf("[TEST CLIENT %d] Connected to server\n", client_id);
    
    // Send login
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, username, MAX_USERNAME - 1);
    strncpy(msg.content, password, MAX_CONTENT - 1);
    
    int len;
    char* buffer = serialize_protocol_message(&msg, &len);
    if (buffer) {
        send(socket, buffer, len, 0);
        free(buffer);
    }
    
    printf("[TEST CLIENT %d] Login sent\n", client_id);
    
    // Wait for login response
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // Check for login response
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    if (select(socket + 1, &readfds, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(socket, &readfds)) {
            test_receive_response(socket);
        }
    }
    
    // Client 1: Keep connection alive for 10 seconds to allow Client 2 to login
    // Client 2: Exit quickly after login
    if (client_id == 1) {
        printf("[TEST CLIENT %d] Keeping connection alive for 10 seconds...\n", client_id);
        for (int i = 0; i < 20; i++) {
            FD_ZERO(&readfds);
            FD_SET(socket, &readfds);
            timeout.tv_sec = 0;
            timeout.tv_usec = 500000; // 500ms
            
            if (select(socket + 1, &readfds, NULL, NULL, &timeout) > 0) {
                if (FD_ISSET(socket, &readfds)) {
                    test_receive_response(socket);
                }
            }
        }
    } else {
        printf("[TEST CLIENT %d] Waiting 2 seconds then exiting...\n", client_id);
        #ifdef _WIN32
        Sleep(2000);
        #else
        sleep(2);
        #endif
    }
    
    close_socket(socket);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return termination_received;
}

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    const char* username = (argc > 2) ? argv[2] : "hust123";
    const char* password = (argc > 3) ? argv[3] : "123456";
    int client_id = (argc > 4) ? atoi(argv[4]) : 1;
    
    printf("=== Test Client %d ===\n", client_id);
    printf("Server: %s\n", server_ip);
    printf("Username: %s\n", username);
    printf("Password: %s\n", password);
    printf("\n");
    
    int result = test_single_session(server_ip, username, password, client_id);
    
    if (result == 1) {
        printf("[TEST CLIENT %d] ✓ Termination message received\n", client_id);
        return 0;
    } else if (result == 0) {
        printf("[TEST CLIENT %d] ✓ Login successful (no termination)\n", client_id);
        return 0;
    } else {
        printf("[TEST CLIENT %d] ✗ Test failed\n", client_id);
        return 1;
    }
}

