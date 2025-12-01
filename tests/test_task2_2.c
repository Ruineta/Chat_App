// Automated test for Task 2.2: Load Offline Messages
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

int receive_and_check_offline(socket_t socket, int* offline_count) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    
    if (select(socket + 1, &readfds, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(socket, &readfds)) {
            int bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                ProtocolMessage* msg = deserialize_protocol_message(buffer, bytes_received);
                if (msg) {
                    if (msg->cmd == CMD_RECEIVE_MESSAGE) {
                        if (strstr(msg->content, "From") != NULL) {
                            (*offline_count)++;
                            printf("Received offline message: %s\n", msg->content);
                        }
                    }
                    free(msg);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    printf("=== Test Task 2.2: Load Offline Messages ===\n");
    
    // Step 1: User1 sends message to User2 (User2 offline)
    printf("\n[Step 1] User1 sends message to User2 (User2 offline)...\n");
    socket_t socket1;
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("Failed to connect\n");
        return 1;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register and login user1
    strncpy(msg.sender, "offline_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket1, &msg);
    
    // Register user2 (but don't login yet)
    strncpy(msg.sender, "offline_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "pass2", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    // Add friend
    strncpy(msg.sender, "offline_user1", MAX_USERNAME - 1);
    strncpy(msg.recipient, "offline_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_ADD_FRIEND;
    send_and_wait(socket1, &msg);
    
    // Send message to user2 (offline)
    strncpy(msg.sender, "offline_user1", MAX_USERNAME - 1);
    strncpy(msg.recipient, "offline_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "This is an offline message", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    msg.is_pinned = false;
    send_and_wait(socket1, &msg);
    printf("Message sent to offline user\n");
    
    close_socket(socket1);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    #ifdef _WIN32
    Sleep(1000);
    #else
    sleep(1);
    #endif
    
    // Step 2: User2 logs in and should receive offline message
    printf("\n[Step 2] User2 logs in (should receive offline message)...\n");
    socket_t socket2;
    if (init_test_client(&socket2, server_ip) < 0) {
        printf("Failed to connect\n");
        return 1;
    }
    
    memset(&msg, 0, sizeof(ProtocolMessage));
    strncpy(msg.sender, "offline_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "pass2", MAX_CONTENT - 1);
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket2, &msg);
    
    // Check for offline messages
    int offline_count = 0;
    for (int i = 0; i < 5; i++) {
        if (receive_and_check_offline(socket2, &offline_count)) {
            // Continue checking
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
    
    printf("\n=== Test Results ===\n");
    if (offline_count > 0) {
        printf("SUCCESS: Received %d offline message(s)\n", offline_count);
        return 0;
    } else {
        printf("FAILED: No offline messages received\n");
        return 1;
    }
}




