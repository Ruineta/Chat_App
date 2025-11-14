#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"  // Includes socket libraries (winsock2.h for Windows, sys/socket.h for Linux)

// Function declarations
int init_client(socket_t* client_socket, const char* server_ip);
void send_command(socket_t socket, ProtocolMessage* msg);
void receive_response(socket_t socket);
void* receive_thread(void* arg);
void print_menu();
void handle_user_input(socket_t socket, const char* username);

#endif // CLIENT_H

