# Makefile for Chat Application
# Supports both Windows (MinGW/MSVC) and Linux

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = 

# Windows specific
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
    SERVER_EXE = build/server.exe
    CLIENT_EXE = build/client.exe
    MKDIR = if not exist
    RMDIR = rmdir /s /q
    RM = del /q
else
    LDFLAGS += -pthread
    SERVER_EXE = build/server
    CLIENT_EXE = build/client
    MKDIR = mkdir -p
    RMDIR = rm -rf
    RM = rm -f
endif

# Source files
COMMON_SRC = src/common.c
SERVER_SRC = src/server.c
CLIENT_SRC = src/client.c
UI_SRC = src/ui.c

# Object files
COMMON_OBJ = build/common.o
SERVER_OBJ = build/server.o $(COMMON_OBJ)
CLIENT_OBJ = build/client.o $(COMMON_OBJ) build/ui.o

# Default target
all: directories $(SERVER_EXE) $(CLIENT_EXE)

# Create necessary directories
directories:
	@$(MKDIR) build 2>nul || mkdir -p build
	@$(MKDIR) data 2>nul || mkdir -p data
	@$(MKDIR) logs 2>nul || mkdir -p logs

# Build server
$(SERVER_EXE): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build client
$(CLIENT_EXE): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile common source
build/common.o: src/common.c include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile server source
build/server.o: src/server.c include/server.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile client source
build/client.o: src/client.c include/client.h include/common.h include/ui.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile UI source
build/ui.o: src/ui.c include/ui.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	$(RM) build/*.o
	$(RM) $(SERVER_EXE) $(CLIENT_EXE)
	$(RMDIR) build 2>nul || true

# Run server (for testing)
run-server: $(SERVER_EXE)
	./$(SERVER_EXE)

# Run client (for testing)
run-client: $(CLIENT_EXE)
	./$(CLIENT_EXE)

.PHONY: all clean run-server run-client directories
