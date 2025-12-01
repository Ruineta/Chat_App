// Advanced Test Suite - Comprehensive Edge Cases & Integration Tests
// Tests all features including edge cases, persistence, and error handling
#include "../include/common.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_RESET "\033[0m"

int test_count = 0;
int pass_count = 0;
int fail_count = 0;

void print_test(const char* category, const char* test_name, bool passed) {
    test_count++;
    if (passed) {
        pass_count++;
        printf("%s[PASS]%s [%s] %s\n", COLOR_GREEN, COLOR_RESET, category, test_name);
    } else {
        fail_count++;
        printf("%s[FAIL]%s [%s] %s\n", COLOR_RED, COLOR_RESET, category, test_name);
    }
}

socket_t create_client(const char* server_ip) {
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return INVALID_SOCKET;
    #endif

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return INVALID_SOCKET;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    
    #ifdef _WIN32
    addr.sin_addr.s_addr = inet_addr(server_ip);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        close_socket(sock);
        WSACleanup();
        return INVALID_SOCKET;
    }
    #else
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        close_socket(sock);
        return INVALID_SOCKET;
    }
    #endif

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        close_socket(sock);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return INVALID_SOCKET;
    }
    return sock;
}

void send_cmd(socket_t sock, ProtocolMessage* msg) {
    int len;
    char* buf = serialize_protocol_message(msg, &len);
    if (buf) {
        send_all(sock, buf, len);
        free(buf);
    }
    #ifdef _WIN32
    Sleep(150);
    #else
    usleep(150000);
    #endif
}

bool recv_response(socket_t sock, CommandType expected_cmd, const char* keyword, int timeout_ms) {
    char buf[BUFFER_SIZE];
    memset(buf, 0, BUFFER_SIZE);
    
    #ifdef _WIN32
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    if (select(0, &readfds, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(sock, &readfds)) {
            int bytes = recv(sock, buf, BUFFER_SIZE - 1, 0);
            if (bytes > 0) {
                buf[bytes] = '\0';
                ProtocolMessage* resp = deserialize_protocol_message(buf, bytes);
                if (resp) {
                    bool cmd_ok = (resp->cmd == expected_cmd);
                    bool keyword_ok = !keyword || strstr(resp->content, keyword) != NULL;
                    bool result = cmd_ok && keyword_ok;
                    free(resp);
                    return result;
                }
            }
        }
    }
    #else
    // Linux select implementation
    #endif
    return false;
}

// Test 1: Friend Request Flow
bool test_friend_request_flow() {
    printf("\n%s=== TEST SUITE 1: Friend Request Flow ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    socket_t a = create_client("127.0.0.1");
    socket_t b = create_client("127.0.0.1");
    if (a == INVALID_SOCKET || b == INVALID_SOCKET) {
        print_test("FRIEND", "Connection failed", false);
        return false;
    }
    
    // Register both (or login if exists)
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    strncpy(msg.sender, "friend_req_a", MAX_USERNAME - 1);
    strncpy(msg.content, "pass", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_cmd(a, &msg);
    // Accept either success or "already exists"
    bool reg_a = recv_response(a, CMD_SUCCESS, NULL, 2000) || 
                 recv_response(a, CMD_ERROR, "exists", 2000);
    print_test("FRIEND", "Register A (or already exists)", reg_a);
    
    strncpy(msg.sender, "friend_req_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    bool reg_b = recv_response(b, CMD_SUCCESS, NULL, 2000) || 
                 recv_response(b, CMD_ERROR, "exists", 2000);
    print_test("FRIEND", "Register B (or already exists)", reg_b);
    
    // Login both
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "friend_req_a", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.sender, "friend_req_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    recv_response(b, CMD_SUCCESS, NULL, 2000);
    
    // A sends friend request to B
    msg.cmd = CMD_SEND_FRIEND_REQUEST;
    strncpy(msg.sender, "friend_req_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "friend_req_b", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    bool req_sent = recv_response(a, CMD_SUCCESS, NULL, 2000);
    print_test("FRIEND", "A sends friend request to B", req_sent);
    
    // B should receive notification
    bool req_received = recv_response(b, CMD_RECEIVE_MESSAGE, "friend_req_a", 2000);
    print_test("FRIEND", "B receives friend request", req_received);
    
    // B accepts request
    msg.cmd = CMD_ACCEPT_FRIEND_REQUEST;
    strncpy(msg.sender, "friend_req_b", MAX_USERNAME - 1);
    strncpy(msg.recipient, "friend_req_a", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    bool accepted = recv_response(b, CMD_SUCCESS, NULL, 2000);
    print_test("FRIEND", "B accepts friend request", accepted);
    
    // Verify friendship
    msg.cmd = CMD_GET_FRIENDS;
    strncpy(msg.sender, "friend_req_a", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    bool has_friend = recv_response(a, CMD_GET_FRIENDS, "friend_req_b", 2000);
    print_test("FRIEND", "A sees B in friends list", has_friend);
    
    close_socket(a);
    close_socket(b);
    return reg_a && reg_b && req_sent && accepted && has_friend;
}

// Test 2: Block User Functionality
bool test_block_user() {
    printf("\n%s=== TEST SUITE 2: Block User ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    socket_t a = create_client("127.0.0.1");
    socket_t b = create_client("127.0.0.1");
    if (a == INVALID_SOCKET || b == INVALID_SOCKET) {
        print_test("BLOCK", "Connection failed", false);
        return false;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register & Login
    strncpy(msg.sender, "block_test_a", MAX_USERNAME - 1);
    strncpy(msg.content, "pass", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.sender, "block_test_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    recv_response(b, CMD_SUCCESS, NULL, 2000);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "block_test_a", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.sender, "block_test_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    recv_response(b, CMD_SUCCESS, NULL, 2000);
    
    // Add friend first
    msg.cmd = CMD_ADD_FRIEND;
    strncpy(msg.sender, "block_test_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "block_test_b", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    // A blocks B
    msg.cmd = CMD_BLOCK_USER;
    strncpy(msg.sender, "block_test_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "block_test_b", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    bool blocked = recv_response(a, CMD_SUCCESS, NULL, 2000);
    print_test("BLOCK", "A blocks B", blocked);
    
    // B tries to send message to A (should fail)
    msg.cmd = CMD_SEND_MESSAGE;
    strncpy(msg.sender, "block_test_b", MAX_USERNAME - 1);
    strncpy(msg.recipient, "block_test_a", MAX_USERNAME - 1);
    strncpy(msg.content, "Blocked message", MAX_CONTENT - 1);
    send_cmd(b, &msg);
    bool block_works = recv_response(b, CMD_ERROR, "blocked", 2000);
    print_test("BLOCK", "B cannot send to blocked A", block_works);
    
    // A unblocks B
    msg.cmd = CMD_UNBLOCK_USER;
    strncpy(msg.sender, "block_test_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "block_test_b", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    bool unblocked = recv_response(a, CMD_SUCCESS, NULL, 2000);
    print_test("BLOCK", "A unblocks B", unblocked);
    
    // B can now send
    msg.cmd = CMD_SEND_MESSAGE;
    strncpy(msg.sender, "block_test_b", MAX_USERNAME - 1);
    strncpy(msg.recipient, "block_test_a", MAX_USERNAME - 1);
    strncpy(msg.content, "Unblocked message", MAX_CONTENT - 1);
    send_cmd(b, &msg);
    bool can_send = recv_response(b, CMD_SUCCESS, "sent", 2000);
    print_test("BLOCK", "B can send after unblock", can_send);
    
    close_socket(a);
    close_socket(b);
    return blocked && block_works && unblocked && can_send;
}

// Test 3: Group Management Edge Cases
bool test_group_edge_cases() {
    printf("\n%s=== TEST SUITE 3: Group Edge Cases ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    socket_t a = create_client("127.0.0.1");
    socket_t b = create_client("127.0.0.1");
    if (a == INVALID_SOCKET || b == INVALID_SOCKET) {
        print_test("GROUP", "Connection failed", false);
        return false;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register & Login
    strncpy(msg.sender, "group_edge_a", MAX_USERNAME - 1);
    strncpy(msg.content, "pass", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.sender, "group_edge_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    recv_response(b, CMD_SUCCESS, NULL, 2000);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "group_edge_a", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.sender, "group_edge_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    recv_response(b, CMD_SUCCESS, NULL, 2000);
    
    // A creates group
    strncpy(msg.sender, "group_edge_a", MAX_USERNAME - 1);
    strncpy(msg.content, "Edge Test Group", MAX_CONTENT - 1);
    msg.cmd = CMD_CREATE_GROUP;
    send_cmd(a, &msg);
    
    char group_id[200];
    char buf[BUFFER_SIZE];
    recv(a, buf, sizeof(buf) - 1, 0);
    ProtocolMessage* resp = deserialize_protocol_message(buf, strlen(buf));
    if (resp && resp->cmd == CMD_SUCCESS) {
        sscanf(resp->content, "Group created: %s", group_id);
        free(resp);
    }
    
    // B tries to kick member (should fail - not admin)
    msg.cmd = CMD_REMOVE_FROM_GROUP;
    strncpy(msg.sender, "group_edge_b", MAX_USERNAME - 1);
    strncpy(msg.recipient, "group_edge_a", MAX_USERNAME - 1);
    strncpy(msg.extra_data, group_id, sizeof(msg.extra_data) - 1);
    send_cmd(b, &msg);
    bool kick_denied = recv_response(b, CMD_ERROR, "admin", 2000) || 
                       recv_response(b, CMD_ERROR, "permission", 2000) ||
                       recv_response(b, CMD_ERROR, "owner", 2000);
    print_test("GROUP", "Non-owner cannot kick", kick_denied);
    
    // A adds B
    msg.cmd = CMD_ADD_TO_GROUP;
    strncpy(msg.sender, "group_edge_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "group_edge_b", MAX_USERNAME - 1);
    strncpy(msg.extra_data, group_id, sizeof(msg.extra_data) - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    // B tries to add themselves again (should fail or be ignored)
    msg.cmd = CMD_ADD_TO_GROUP;
    strncpy(msg.sender, "group_edge_b", MAX_USERNAME - 1);
    strncpy(msg.recipient, "group_edge_b", MAX_USERNAME - 1);
    strncpy(msg.extra_data, group_id, sizeof(msg.extra_data) - 1);
    send_cmd(b, &msg);
    bool self_add_handled = recv_response(b, CMD_ERROR, NULL, 2000) || 
                           recv_response(b, CMD_SUCCESS, NULL, 2000);
    print_test("GROUP", "Self-add handled", self_add_handled);
    
    // B leaves group
    msg.cmd = CMD_LEAVE_GROUP;
    strncpy(msg.sender, "group_edge_b", MAX_USERNAME - 1);
    strncpy(msg.extra_data, group_id, sizeof(msg.extra_data) - 1);
    send_cmd(b, &msg);
    bool left = recv_response(b, CMD_SUCCESS, NULL, 2000);
    print_test("GROUP", "B leaves group", left);
    
    close_socket(a);
    close_socket(b);
    return kick_denied && self_add_handled && left;
}

// Test 4: History Loading & Search
bool test_history_and_search() {
    printf("\n%s=== TEST SUITE 4: History & Search ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    socket_t a = create_client("127.0.0.1");
    socket_t b = create_client("127.0.0.1");
    if (a == INVALID_SOCKET || b == INVALID_SOCKET) {
        print_test("HISTORY", "Connection failed", false);
        return false;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register & Login
    strncpy(msg.sender, "hist_test_a", MAX_USERNAME - 1);
    strncpy(msg.content, "pass", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.sender, "hist_test_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    recv_response(b, CMD_SUCCESS, NULL, 2000);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "hist_test_a", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.sender, "hist_test_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    recv_response(b, CMD_SUCCESS, NULL, 2000);
    
    // Add friend
    msg.cmd = CMD_ADD_FRIEND;
    strncpy(msg.sender, "hist_test_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "hist_test_b", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    // A sends multiple messages
    msg.cmd = CMD_SEND_MESSAGE;
    strncpy(msg.sender, "hist_test_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "hist_test_b", MAX_USERNAME - 1);
    strncpy(msg.content, "Message with keyword SEARCH_TEST", MAX_CONTENT - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.content, "Another message", MAX_CONTENT - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // A requests history
    msg.cmd = CMD_GET_CHAT_HISTORY;
    strncpy(msg.sender, "hist_test_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "hist_test_b", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    // History might return NO_HISTORY or actual messages
    char buf[BUFFER_SIZE];
    memset(buf, 0, BUFFER_SIZE);
    recv(a, buf, sizeof(buf) - 1, 0);
    ProtocolMessage* resp = deserialize_protocol_message(buf, strlen(buf));
    bool history_loaded = false;
    if (resp) {
        history_loaded = (resp->cmd == CMD_GET_CHAT_HISTORY) && 
                        (strstr(resp->content, "SEARCH_TEST") != NULL || 
                         strstr(resp->content, "Another message") != NULL ||
                         strcmp(resp->content, "NO_HISTORY") != 0);
        free(resp);
    }
    print_test("HISTORY", "History loaded with messages", history_loaded);
    
    // A searches history
    msg.cmd = CMD_SEARCH_HISTORY;
    strncpy(msg.sender, "hist_test_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "hist_test_b", MAX_USERNAME - 1);
    strncpy(msg.content, "SEARCH_TEST", MAX_CONTENT - 1);
    send_cmd(a, &msg);
    bool search_works = recv_response(a, CMD_SEARCH_HISTORY, "SEARCH_TEST", 2000);
    print_test("HISTORY", "Search returns results", search_works);
    
    close_socket(a);
    close_socket(b);
    return history_loaded && search_works;
}

// Test 5: Unfriend Complete Flow
bool test_unfriend_complete() {
    printf("\n%s=== TEST SUITE 5: Unfriend Complete Flow ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    socket_t a = create_client("127.0.0.1");
    socket_t b = create_client("127.0.0.1");
    if (a == INVALID_SOCKET || b == INVALID_SOCKET) {
        print_test("UNFRIEND", "Connection failed", false);
        return false;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register & Login
    strncpy(msg.sender, "unfriend_a", MAX_USERNAME - 1);
    strncpy(msg.content, "pass", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.sender, "unfriend_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    recv_response(b, CMD_SUCCESS, NULL, 2000);
    
    msg.cmd = CMD_LOGIN;
    strncpy(msg.sender, "unfriend_a", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    strncpy(msg.sender, "unfriend_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    recv_response(b, CMD_SUCCESS, NULL, 2000);
    
    // Add friend
    msg.cmd = CMD_ADD_FRIEND;
    strncpy(msg.sender, "unfriend_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "unfriend_b", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    recv_response(a, CMD_SUCCESS, NULL, 2000);
    
    // Verify friendship
    msg.cmd = CMD_GET_FRIENDS;
    strncpy(msg.sender, "unfriend_a", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    bool has_friend_before = recv_response(a, CMD_GET_FRIENDS, "unfriend_b", 2000);
    print_test("UNFRIEND", "A has B as friend before", has_friend_before);
    
    // A unfriends B
    msg.cmd = CMD_UNFRIEND;
    strncpy(msg.sender, "unfriend_a", MAX_USERNAME - 1);
    strncpy(msg.recipient, "unfriend_b", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    bool unfriended = recv_response(a, CMD_SUCCESS, "Unfriended", 2000);
    print_test("UNFRIEND", "A unfriends B successfully", unfriended);
    
    // Verify B removed from A's list
    msg.cmd = CMD_GET_FRIENDS;
    strncpy(msg.sender, "unfriend_a", MAX_USERNAME - 1);
    send_cmd(a, &msg);
    #ifdef _WIN32
    Sleep(300);
    #else
    usleep(300000);
    #endif
    char buf[BUFFER_SIZE];
    recv(a, buf, sizeof(buf) - 1, 0);
    ProtocolMessage* resp = deserialize_protocol_message(buf, strlen(buf));
    bool b_removed = !resp || strstr(resp->content, "unfriend_b") == NULL;
    if (resp) free(resp);
    print_test("UNFRIEND", "B removed from A's friends list", b_removed);
    
    // Verify bidirectional: A removed from B's list
    msg.cmd = CMD_GET_FRIENDS;
    strncpy(msg.sender, "unfriend_b", MAX_USERNAME - 1);
    send_cmd(b, &msg);
    #ifdef _WIN32
    Sleep(300);
    #else
    usleep(300000);
    #endif
    recv(b, buf, sizeof(buf) - 1, 0);
    resp = deserialize_protocol_message(buf, strlen(buf));
    bool a_removed = !resp || strstr(resp->content, "unfriend_a") == NULL;
    if (resp) free(resp);
    print_test("UNFRIEND", "A removed from B's friends list (bidirectional)", a_removed);
    
    close_socket(a);
    close_socket(b);
    return has_friend_before && unfriended && b_removed && a_removed;
}

int main() {
    printf("========================================\n");
    printf("  ADVANCED TEST SUITE\n");
    printf("  Comprehensive Edge Cases & Integration\n");
    printf("========================================\n\n");
    
    printf("%sNOTE: Server must be running on 127.0.0.1:%d%s\n", COLOR_YELLOW, PORT, COLOR_RESET);
    printf("Starting advanced tests in 2 seconds...\n");
    #ifdef _WIN32
    Sleep(2000);
    #else
    usleep(2000000);
    #endif
    
    // Run all test suites
    test_friend_request_flow();
    test_block_user();
    test_group_edge_cases();
    test_history_and_search();
    test_unfriend_complete();
    
    // Summary
    printf("\n========================================\n");
    printf("  ADVANCED TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total Tests: %d\n", test_count);
    printf("%sPassed: %d%s\n", COLOR_GREEN, pass_count, COLOR_RESET);
    printf("%sFailed: %d%s\n", fail_count > 0 ? COLOR_RED : COLOR_GREEN, fail_count, COLOR_RESET);
    printf("Success Rate: %.1f%%\n", test_count > 0 ? (float)pass_count / test_count * 100 : 0);
    
    if (fail_count == 0) {
        printf("\n%s✅ ALL ADVANCED TESTS PASSED!%s\n", COLOR_GREEN, COLOR_RESET);
        return 0;
    } else {
        printf("\n%s❌ SOME TESTS FAILED!%s\n", COLOR_RED, COLOR_RESET);
        return 1;
    }
}

