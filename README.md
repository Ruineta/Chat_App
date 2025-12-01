# Chat Application - Network Programming Project

A TCP-based chat application with multithreading support, implementing real-time messaging, group chats, and advanced features.

## Features

### Core Features
- **User Management**: Registration, login, friend management
- **Real-time 1-1 Chat**: Instant messaging between users
- **Group Chat**: Create groups, add members, group messaging
- **Message Persistence**: Messages saved to file, history loading
- **Offline Messaging**: Messages delivered when user comes online
- **Activity Logging**: All actions logged to file
- **Search History**: Search messages by keyword
- **Block/Unblock Users**: User blocking functionality
- **Pin Messages**: Pin important messages in groups
- **UTF-8 Support**: Vietnamese characters and emoji support

## Project Structure

```
Chat_App/
├── src/              # Source code files
│   ├── server.c      # Server implementation
│   ├── client.c      # Client implementation
│   ├── common.c      # Shared utilities
│   └── ui.c          # User interface
├── include/          # Header files
│   ├── server.h
│   ├── client.h
│   ├── common.h
│   └── ui.h
├── data/             # Data files (created at runtime)
│   ├── account.txt
│   └── messages.txt
├── logs/             # Log files (created at runtime)
│   └── activity.log
├── build/            # Build artifacts (created at compile time)
├── tests/            # Test files
├── docs/             # Documentation
├── Makefile          # Build configuration
└── README.md         # This file
```

## Requirements

- **OS**: Windows 10+ or Linux
- **Compiler**: GCC (MinGW on Windows)
- **Libraries**: 
  - Windows: `ws2_32` (Winsock)
  - Linux: `pthread`

## Building

### Using Makefile

```bash
# Compile project
make

# Clean build files
make clean
```

### Manual Compilation

**Windows:**
```bash
gcc -Wall -Wextra -std=c11 -Iinclude -c src/common.c -o build/common.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/server.c -o build/server.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/client.c -o build/client.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/ui.c -o build/ui.o
gcc -Wall -Wextra -std=c11 -Iinclude -o build/server.exe build/server.o build/common.o -lws2_32
gcc -Wall -Wextra -std=c11 -Iinclude -o build/client.exe build/client.o build/common.o build/ui.o -lws2_32
```

**Linux:**
```bash
gcc -Wall -Wextra -std=c11 -Iinclude -c src/common.c -o build/common.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/server.c -o build/server.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/client.c -o build/client.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/ui.c -o build/ui.o
gcc -Wall -Wextra -std=c11 -Iinclude -o build/server build/server.o build/common.o -pthread
gcc -Wall -Wextra -std=c11 -Iinclude -o build/client build/client.o build/common.o build/ui.o -pthread
```

## Running

### Start Server

```bash
cd build
./server.exe    # Windows
./server         # Linux
```

### Start Client

```bash
cd build
./client.exe     # Windows
./client         # Linux
```

## Quick Start

1. **Compile the project:**
   ```bash
   make
   ```

2. **Start server** (Terminal 1):
   ```bash
   cd build
   ./server.exe
   ```

3. **Start client** (Terminal 2):
   ```bash
   cd build
   ./client.exe
   ```

4. **Register and login:**
   - Choose option `1` to register
   - Choose option `2` to login

5. **Start chatting:**
   - Add friend: option `4`
   - Chat 1-1: option `11`
   - Create group: option `7`

## Configuration

- **Port**: Default port is `8080` (defined in `include/common.h`)
- **Data Directory**: `data/` (relative to executable)
- **Log Directory**: `logs/` (relative to executable)

## Protocol

The application uses a custom text-based protocol:
```
CMD:<command_type>|SENDER:<username>|RECIPIENT:<recipient>|CONTENT:<content>|EXTRA:<extra_data>|TYPE:<message_type>|PINNED:<0|1>|
```

## Notes

- Server supports multiple concurrent clients using multithreading
- Messages are stored in `data/messages.txt` for persistence
- Activity logs are written to `logs/activity.log`
- Offline messages are stored and delivered when users come online
- Group administrators can manage group members and settings

## Development

### Code Structure
- **Server**: Handles client connections, message routing, persistence
- **Client**: User interface, input handling, server communication
- **Common**: Shared utilities, protocol serialization, logging
- **UI**: Console UI framework with chat bubbles, colors, UTF-8 support

### Thread Safety
- Server uses mutexes for shared state (Windows: CRITICAL_SECTION, Linux: pthread_mutex)
- File I/O operations are protected by mutexes
- Activity logging is thread-safe

## License

This project is part of a Network Programming course assignment.

## Contributors

- Project team members
