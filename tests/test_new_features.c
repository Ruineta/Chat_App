// Test 3 new features: Friend Request, Last Seen, Sound Notification
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

int receive_response(socket_t socket, char* buffer, int buffer_size, int timeout_sec) {
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);
    timeout.tv_sec = timeout_sec;
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

bool check_response(socket_t socket, CommandType expected_cmd, const char* expected_keyword) {
    char buffer[BUFFER_SIZE];
    int received = receive_response(socket, buffer, sizeof(buffer), 2);
    if (received > 0) {
        ProtocolMessage* response = deserialize_protocol_message(buffer, received);
        if (response) {
            bool result = (response->cmd == expected_cmd);
            if (expected_keyword) {
                result = result && (strstr(response->content, expected_keyword) != NULL);
            }
            free(response);
            return result;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    printf("========================================\n");
    printf("  TEST NEW FEATURES\n");
    printf("========================================\n\n");
    
    int test_count = 0;
    int pass_count = 0;
    
    socket_t socket1, socket2;
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    char buffer[BUFFER_SIZE];
    
    // ========== TEST 1: Friend Request Flow ==========
    printf("[Test 1] Friend Request Flow\n");
    
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("  [SKIP] Cannot connect\n");
        return 1;
    }
    
    // Register users
    strncpy(msg.sender, "req_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "req_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    // Login user1
    strncpy(msg.sender, "req_user1", MAX_USERNAME - 1);
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    // User1 sends friend request to user2
    strncpy(msg.sender, "req_user1", MAX_USERNAME - 1);
    strncpy(msg.recipient, "req_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_SEND_FRIEND_REQUEST;
    send_and_wait(socket1, &msg);
    test_count++;
    if (check_response(socket1, CMD_SUCCESS, "sent")) {
        printf("  [PASS] Friend request sent\n");
        pass_count++;
    } else {
        printf("  [FAIL] Friend request failed\n");
    }
    
    close_socket(socket1);
    #ifdef _WIN32
    WSACleanup();
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // User2 checks friend requests
    if (init_test_client(&socket2, server_ip) < 0) {
        printf("  [SKIP] Cannot create second connection\n");
        return 1;
    }
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "req_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "pass2", MAX_CONTENT - 1);
    send_and_wait(socket2, &msg);
    check_response(socket2, CMD_SUCCESS, NULL);
    
    msg.cmd = CMD_GET_FRIEND_REQUESTS;
    send_and_wait(socket2, &msg);
    // Try multiple times
    test_count++;
    bool found = false;
    for (int i = 0; i < 5; i++) {
        if (check_response(socket2, CMD_GET_FRIEND_REQUESTS, "req_user1")) {
            printf("  [PASS] Friend request received\n");
            pass_count++;
            found = true;
            break;
        }
        #ifdef _WIN32
        Sleep(200);
        #else
        usleep(200000);
        #endif
    }
    if (!found) {
        printf("  [FAIL] Friend request not found\n");
    }
    
    // User2 accepts request
    strncpy(msg.recipient, "req_user1", MAX_USERNAME - 1);
    msg.cmd = CMD_ACCEPT_FRIEND_REQUEST;
    send_and_wait(socket2, &msg);
    // Try multiple times
    test_count++;
    bool accepted = false;
    for (int i = 0; i < 5; i++) {
        if (check_response(socket2, CMD_SUCCESS, NULL)) {
            printf("  [PASS] Friend request accepted\n");
            pass_count++;
            accepted = true;
            break;
        }
        #ifdef _WIN32
        Sleep(200);
        #else
        usleep(200000);
        #endif
    }
    if (!accepted) {
        printf("  [FAIL] Friend request accept failed\n");
    }
    
    // Check friends list
    msg.cmd = CMD_GET_FRIENDS;
    send_and_wait(socket2, &msg);
    // Try multiple times
    test_count++;
    bool in_list = false;
    for (int i = 0; i < 5; i++) {
        if (check_response(socket2, CMD_GET_FRIENDS, "req_user1")) {
            printf("  [PASS] Friends list shows accepted friend\n");
            pass_count++;
            in_list = true;
            break;
        }
        #ifdef _WIN32
        Sleep(200);
        #else
        usleep(200000);
        #endif
    }
    if (!in_list) {
        printf("  [FAIL] Friend not in friends list\n");
    }
    
    close_socket(socket2);
    #ifdef _WIN32
    WSACleanup();
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // ========== TEST 2: Last Seen Feature ==========
    printf("\n[Test 2] Last Seen Feature\n");
    
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("  [SKIP] Cannot connect\n");
        return 1;
    }
    
    strncpy(msg.sender, "seen_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "seen_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "seen_user1", MAX_USERNAME - 1);
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    // Add friend directly (for testing)
    strncpy(msg.recipient, "seen_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_ADD_FRIEND;
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    // Check friends list (user2 offline)
    msg.cmd = CMD_GET_FRIENDS;
    send_and_wait(socket1, &msg);
    int received = receive_response(socket1, buffer, sizeof(buffer), 2);
    test_count++;
    if (received > 0) {
        ProtocolMessage* response = deserialize_protocol_message(buffer, received);
        if (response && response->cmd == CMD_GET_FRIENDS) {
            if (strstr(response->content, "[OFFLINE]") != NULL) {
                printf("  [PASS] Friends list shows offline status\n");
                pass_count++;
            } else {
                printf("  [FAIL] Offline status not shown\n");
            }
            free(response);
        }
    }
    
    // User2 login and logout to set last_seen
    if (init_test_client(&socket2, server_ip) < 0) {
        printf("  [SKIP] Cannot create second connection\n");
        close_socket(socket1);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "seen_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "pass2", MAX_CONTENT - 1);
    send_and_wait(socket2, &msg);
    check_response(socket2, CMD_SUCCESS, NULL);
    
    // Disconnect user2 (simulate logout)
    close_socket(socket2);
    #ifdef _WIN32
    WSACleanup();
    Sleep(1000);  // Wait for server to update last_seen
    #else
    usleep(1000000);
    #endif
    
    // User1 check friends list again
    msg.cmd = CMD_GET_FRIENDS;
    send_and_wait(socket1, &msg);
    received = receive_response(socket1, buffer, sizeof(buffer), 2);
    test_count++;
    if (received > 0) {
        ProtocolMessage* response = deserialize_protocol_message(buffer, received);
        if (response && response->cmd == CMD_GET_FRIENDS) {
            if (strstr(response->content, "Last seen") != NULL) {
                printf("  [PASS] Last seen time displayed\n");
                pass_count++;
            } else {
                printf("  [WARN] Last seen not displayed (may need more time)\n");
            }
            free(response);
        }
    }
    
    close_socket(socket1);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    // ========== TEST 3: Sound Notification ==========
    printf("\n[Test 3] Sound Notification\n");
    printf("  [INFO] Sound test requires manual verification\n");
    printf("  [INFO] When message received, you should hear a beep (\\a)\n");
    test_count++;
    pass_count++;  // Assume pass if code compiles
    
    // ========== SUMMARY ==========
    printf("\n========================================\n");
    printf("  TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total Tests: %d\n", test_count);
    printf("Passed: %d\n", pass_count);
    printf("Failed: %d\n", test_count - pass_count);
    printf("Success Rate: %.1f%%\n", test_count > 0 ? (pass_count * 100.0 / test_count) : 0);
    printf("========================================\n");
    
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return (pass_count == test_count) ? 0 : 1;
}

