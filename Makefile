# Makefile for Chat Application
# Supports both Windows (MinGW/MSVC) and Linux

CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = 

# Windows specific
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
    SERVER_EXE = server.exe
    CLIENT_EXE = client.exe
else
    LDFLAGS += -pthread
    SERVER_EXE = server
    CLIENT_EXE = client
endif

# Source files
COMMON_SRC = common.c
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Object files
COMMON_OBJ = $(COMMON_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o) $(COMMON_OBJ)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o) $(COMMON_OBJ)

# Default target
all: $(SERVER_EXE) $(CLIENT_EXE)

# Build server
$(SERVER_EXE): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build client
$(CLIENT_EXE): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile common source
common.o: common.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile server source
server.o: server.c server.h common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile client source
client.o: client.c client.h common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f *.o $(SERVER_EXE) $(CLIENT_EXE) activity.log messages.txt

# Run server (for testing)
run-server: $(SERVER_EXE)
	./$(SERVER_EXE)

# Run client (for testing)
run-client: $(CLIENT_EXE)
	./$(CLIENT_EXE)

.PHONY: all clean run-server run-client

