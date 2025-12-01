// Comprehensive Test Suite for Chat Application
// Tests all implemented features from Phase 1, 2, and 3
#include "../include/common.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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

int test_count = 0;
int pass_count = 0;
int fail_count = 0;

void test_result(const char* test_name, bool passed) {
    test_count++;
    if (passed) {
        pass_count++;
        printf("  [PASS] %s\n", test_name);
    } else {
        fail_count++;
        printf("  [FAIL] %s\n", test_name);
    }
}

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    printf("========================================\n");
    printf("  COMPREHENSIVE TEST SUITE\n");
    printf("  Chat Application - All Features\n");
    printf("========================================\n\n");
    
    socket_t socket1, socket2;
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    char buffer[BUFFER_SIZE];
    
    // ========== PHASE 1 TESTS ==========
    printf("=== PHASE 1: Core Refinement ===\n");
    
    // Test 1.1: Single Session Enforcement
    printf("\n[Test 1.1] Single Session Enforcement\n");
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("  [SKIP] Cannot connect to server\n");
        return 1;
    }
    
    strncpy(msg.sender, "session_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket1, &msg);
    bool login1_ok = check_response(socket1, CMD_SUCCESS, "Login successful");
    test_result("First login successful", login1_ok);
    
    // Second login from different socket
    if (init_test_client(&socket2, server_ip) < 0) {
        printf("  [SKIP] Cannot create second connection\n");
        close_socket(socket1);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }
    
    strncpy(msg.sender, "session_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket2, &msg);
    bool login2_ok = check_response(socket2, CMD_SUCCESS, "Login successful");
    test_result("Second login successful (should terminate first)", login2_ok);
    
    // Check if first socket received termination message
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    bool termination_received = receive_response(socket1, buffer, sizeof(buffer), 1) > 0;
    test_result("First session received termination message", termination_received);
    
    close_socket(socket1);
    close_socket(socket2);
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // Test 1.2: Stream Handling (send_all/recv_all)
    printf("\n[Test 1.2] Stream Handling (Reliable Send/Receive)\n");
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("  [SKIP] Cannot connect\n");
        return 1;
    }
    
    strncpy(msg.sender, "stream_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "stream_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "stream_user1", MAX_USERNAME - 1);
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    strncpy(msg.recipient, "stream_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "Test message for stream handling", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    send_and_wait(socket1, &msg);
    bool stream_ok = check_response(socket1, CMD_SUCCESS, "Message sent");
    test_result("Message sent successfully (stream handling)", stream_ok);
    
    close_socket(socket1);
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // ========== PHASE 2 TESTS ==========
    printf("\n=== PHASE 2: Message Persistence ===\n");
    
    // Test 2.1: Messages.txt Format with Pin Support
    printf("\n[Test 2.1] Messages.txt Format with Pin Support\n");
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("  [SKIP] Cannot connect\n");
        return 1;
    }
    
    strncpy(msg.sender, "format_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "format_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "format_user1", MAX_USERNAME - 1);
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    strncpy(msg.recipient, "format_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "Pinned message test", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    msg.is_pinned = true;
    send_and_wait(socket1, &msg);
    bool pin_sent = check_response(socket1, CMD_SUCCESS, NULL);
    test_result("Pinned message sent", pin_sent);
    
    close_socket(socket1);
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // Test 2.2: Offline Messages
    printf("\n[Test 2.2] Offline Messages\n");
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("  [SKIP] Cannot connect\n");
        return 1;
    }
    
    strncpy(msg.sender, "offline_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "offline_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "offline_user1", MAX_USERNAME - 1);
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    strncpy(msg.recipient, "offline_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "Offline message test", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    msg.is_pinned = false;
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    close_socket(socket1);
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // Now login as offline_user2 and check for offline message
    if (init_test_client(&socket2, server_ip) < 0) {
        printf("  [SKIP] Cannot create second connection\n");
        return 1;
    }
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "offline_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "pass2", MAX_CONTENT - 1);
    send_and_wait(socket2, &msg);
    check_response(socket2, CMD_SUCCESS, NULL);
    
    // Check for offline message - try multiple times
    bool offline_received = false;
    for (int i = 0; i < 5; i++) {
        #ifdef _WIN32
        Sleep(500);
        #else
        usleep(500000);
        #endif
        int received = receive_response(socket2, buffer, sizeof(buffer), 2);
        if (received > 0) {
            ProtocolMessage* response = deserialize_protocol_message(buffer, received);
            if (response && response->cmd == CMD_RECEIVE_MESSAGE) {
                if (strstr(response->content, "Offline message test") != NULL) {
                    offline_received = true;
                    free(response);
                    break;
                }
            }
            if (response) free(response);
        }
    }
    test_result("Offline message received on login", offline_received);
    
    close_socket(socket2);
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // Test 2.4: Search History
    printf("\n[Test 2.4] Search History Function\n");
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("  [SKIP] Cannot connect\n");
        return 1;
    }
    
    strncpy(msg.sender, "search_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "search_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "search_user1", MAX_USERNAME - 1);
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    strncpy(msg.recipient, "search_user2", MAX_USERNAME - 1);
    strncpy(msg.content, "Test keyword search", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    strncpy(msg.content, "keyword", MAX_CONTENT - 1);
    strncpy(msg.recipient, "search_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_SEARCH_HISTORY;
    send_and_wait(socket1, &msg);
    bool search_ok = check_response(socket1, CMD_SEARCH_HISTORY, "keyword");
    test_result("Search history returns results", search_ok);
    
    close_socket(socket1);
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // Test 2.5: Block User Logic
    printf("\n[Test 2.5] Block User Logic\n");
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("  [SKIP] Cannot connect\n");
        return 1;
    }
    
    strncpy(msg.sender, "block_userA", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "block_userB", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "block_userA", MAX_USERNAME - 1);
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    strncpy(msg.recipient, "block_userB", MAX_USERNAME - 1);
    msg.cmd = CMD_BLOCK_USER;
    send_and_wait(socket1, &msg);
    bool block_ok = check_response(socket1, CMD_SUCCESS, NULL);
    test_result("UserA blocked UserB", block_ok);
    
    close_socket(socket1);
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // UserB tries to send to UserA (should be rejected)
    if (init_test_client(&socket2, server_ip) < 0) {
        printf("  [SKIP] Cannot create second connection\n");
        return 1;
    }
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "block_userB", MAX_USERNAME - 1);
    strncpy(msg.content, "pass2", MAX_CONTENT - 1);
    send_and_wait(socket2, &msg);
    check_response(socket2, CMD_SUCCESS, NULL);
    
    strncpy(msg.recipient, "block_userA", MAX_USERNAME - 1);
    strncpy(msg.content, "Test blocked message", MAX_CONTENT - 1);
    msg.cmd = CMD_SEND_MESSAGE;
    msg.msg_type = MSG_TEXT;
    send_and_wait(socket2, &msg);
    // Check response - try multiple times
    bool block_rejected = false;
    for (int i = 0; i < 5; i++) {
        int received = receive_response(socket2, buffer, sizeof(buffer), 2);
        if (received > 0) {
            ProtocolMessage* response = deserialize_protocol_message(buffer, received);
            if (response && response->cmd == CMD_ERROR && strstr(response->content, "blocked") != NULL) {
                block_rejected = true;
                free(response);
                break;
            }
            if (response) free(response);
        }
        #ifdef _WIN32
        Sleep(200);
        #else
        usleep(200000);
        #endif
    }
    test_result("Blocked user cannot send message", block_rejected);
    
    close_socket(socket2);
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // ========== PHASE 3 TESTS ==========
    printf("\n=== PHASE 3: UI/UX Improvements ===\n");
    
    // Test 3.2: Friends List with Online/Offline Status
    printf("\n[Test 3.2] Friends List with Online/Offline Status\n");
    if (init_test_client(&socket1, server_ip) < 0) {
        printf("  [SKIP] Cannot connect\n");
        return 1;
    }
    
    strncpy(msg.sender, "friend_user1", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    strncpy(msg.sender, "friend_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket1, &msg);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "friend_user1", MAX_USERNAME - 1);
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    strncpy(msg.recipient, "friend_user2", MAX_USERNAME - 1);
    msg.cmd = CMD_ADD_FRIEND;
    send_and_wait(socket1, &msg);
    check_response(socket1, CMD_SUCCESS, NULL);
    
    msg.cmd = CMD_GET_FRIENDS;
    send_and_wait(socket1, &msg);
    // Check response - try multiple times
    bool friends_ok = false;
    for (int i = 0; i < 5; i++) {
        int received = receive_response(socket1, buffer, sizeof(buffer), 2);
        if (received > 0) {
            ProtocolMessage* response = deserialize_protocol_message(buffer, received);
            if (response && response->cmd == CMD_GET_FRIENDS) {
                if (strstr(response->content, "[ONLINE]") != NULL || strstr(response->content, "[OFFLINE]") != NULL) {
                    friends_ok = true;
                    free(response);
                    break;
                }
            }
            if (response) free(response);
        }
        #ifdef _WIN32
        Sleep(200);
        #else
        usleep(200000);
        #endif
    }
    test_result("Friends list shows online/offline status", friends_ok);
    
    close_socket(socket1);
    
    // ========== SUMMARY ==========
    printf("\n========================================\n");
    printf("  TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total Tests: %d\n", test_count);
    printf("Passed: %d\n", pass_count);
    printf("Failed: %d\n", fail_count);
    printf("Success Rate: %.1f%%\n", test_count > 0 ? (pass_count * 100.0 / test_count) : 0);
    printf("========================================\n");
    
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return (fail_count == 0) ? 0 : 1;
}

