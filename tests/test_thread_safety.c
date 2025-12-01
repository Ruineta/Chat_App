// Test Thread Safety - Multiple clients sending messages simultaneously
#include "../include/common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
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
    Sleep(50);  // Shorter delay for concurrent testing
    #else
    usleep(50000);
    #endif
}

// Thread function to send multiple messages
#ifdef _WIN32
DWORD WINAPI send_messages_thread(LPVOID param) {
#else
void* send_messages_thread(void* param) {
#endif
    struct {
        socket_t socket;
        const char* server_ip;
        const char* username;
        const char* password;
        const char* recipient;
        int message_count;
    } *args = (struct {
        socket_t socket;
        const char* server_ip;
        const char* username;
        const char* password;
        const char* recipient;
        int message_count;
    }*)param;
    
    socket_t socket;
    if (init_test_client(&socket, args->server_ip) < 0) {
        printf("Thread %s: Failed to connect\n", args->username);
        return 0;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    // Register
    strncpy(msg.sender, args->username, MAX_USERNAME - 1);
    strncpy(msg.content, args->password, MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(socket, &msg);
    
    // Login
    msg.cmd = CMD_LOGIN;
    send_and_wait(socket, &msg);
    
    // Send multiple messages
    for (int i = 0; i < args->message_count; i++) {
        char content[200];
        snprintf(content, sizeof(content), "Thread safety test message %d from %s", i, args->username);
        
        strncpy(msg.sender, args->username, MAX_USERNAME - 1);
        strncpy(msg.recipient, args->recipient, MAX_USERNAME - 1);
        strncpy(msg.content, content, MAX_CONTENT - 1);
        msg.cmd = CMD_SEND_MESSAGE;
        msg.msg_type = MSG_TEXT;
        msg.is_pinned = false;
        send_and_wait(socket, &msg);
    }
    
    close_socket(socket);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return 0;
}

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    int num_threads = (argc > 2) ? atoi(argv[2]) : 5;
    int messages_per_thread = (argc > 3) ? atoi(argv[3]) : 10;
    
    printf("=== Thread Safety Test ===\n");
    printf("Server: %s\n", server_ip);
    printf("Threads: %d\n", num_threads);
    printf("Messages per thread: %d\n", messages_per_thread);
    printf("Total messages: %d\n\n", num_threads * messages_per_thread);
    
    // First, create a recipient user
    socket_t recipient_socket;
    if (init_test_client(&recipient_socket, server_ip) < 0) {
        printf("Failed to connect for recipient\n");
        return 1;
    }
    
    ProtocolMessage msg;
    memset(&msg, 0, sizeof(ProtocolMessage));
    
    strncpy(msg.sender, "thread_recipient", MAX_USERNAME - 1);
    strncpy(msg.content, "pass1", MAX_CONTENT - 1);
    msg.cmd = CMD_REGISTER;
    send_and_wait(recipient_socket, &msg);
    
    close_socket(recipient_socket);
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    #ifdef _WIN32
    Sleep(500);
    #else
    usleep(500000);
    #endif
    
    // Prepare thread arguments
    struct {
        socket_t socket;
        const char* server_ip;
        const char* username;
        const char* password;
        const char* recipient;
        int message_count;
    } *thread_args = malloc(num_threads * sizeof(*thread_args));
    
    #ifdef _WIN32
    HANDLE* threads = malloc(num_threads * sizeof(HANDLE));
    #else
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    #endif
    
    for (int i = 0; i < num_threads; i++) {
        char username[50];
        snprintf(username, sizeof(username), "thread_user%d", i);
        
        thread_args[i].socket = INVALID_SOCKET;
        thread_args[i].server_ip = server_ip;
        thread_args[i].username = strdup(username);
        thread_args[i].password = "pass1";
        thread_args[i].recipient = "thread_recipient";
        thread_args[i].message_count = messages_per_thread;
    }
    
    printf("Starting %d threads...\n", num_threads);
    
    // Start all threads
    for (int i = 0; i < num_threads; i++) {
        #ifdef _WIN32
        threads[i] = CreateThread(NULL, 0, send_messages_thread, &thread_args[i], 0, NULL);
        if (threads[i] == NULL) {
            printf("Failed to create thread %d\n", i);
        }
        #else
        if (pthread_create(&threads[i], NULL, send_messages_thread, &thread_args[i]) != 0) {
            printf("Failed to create thread %d\n", i);
        }
        #endif
    }
    
    printf("Waiting for all threads to complete...\n");
    
    // Wait for all threads
    for (int i = 0; i < num_threads; i++) {
        #ifdef _WIN32
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
        #else
        pthread_join(threads[i], NULL);
        #endif
    }
    
    printf("\nAll threads completed.\n");
    printf("Please check messages.txt and activity.log for corruption.\n");
    printf("Expected: %d messages should be written correctly.\n", num_threads * messages_per_thread);
    
    // Cleanup
    for (int i = 0; i < num_threads; i++) {
        free((void*)thread_args[i].username);
    }
    free(thread_args);
    free(threads);
    
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return 0;
}




