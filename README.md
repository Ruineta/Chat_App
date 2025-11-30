# Chat Application

A simple TCP-based chat application with multithreading support, implementing all required features from the project rubric.

## Features

### Core Features (14 points total)

1. **Get Friend List and Status** (1 point)
   - Display list of users and their online/offline status

2. **Send and Receive Messages (1-1)** (1 point)
   - Real-time 1-1 chat between two users

3. **Disconnect** (1 point)
   - When a user leaves a conversation, the system notifies the other party

4. **Create Group Chat** (1 point)
   - Allows users to create new group chats

5. **Add Users to Group Chat** (1 point)
   - Group creator can invite more members

6. **Remove Users from Group Chat** (1 point)
   - Group administrators can remove members from the group

7. **Leave Group Chat** (1 point)
   - Users can leave groups they are participating in

8. **Send and Receive Messages in Group Chat** (1 point)
   - Supports sending messages to all members in the group

9. **Send Offline Messages** (1 point)
   - Saves messages when the recipient is not yet online

10. **Log Activity** (1 point)
    - Records all user activity for review

11. **Search Chat History** (1 point)
    - Allows searching old messages by keyword

12. **Send Emoji** (1 point)
    - Supports emojis in messages

13. **Set Group Nickname** (0.5 points)
    - Rename group chat

14. **Block User** (1 point)
    - Block users

15. **Pin Message in Conversation** (0.5 points)
    - Pin messages in chat segment

## Architecture

- **Server**: Multithreaded TCP server handling multiple client connections
- **Client**: TCP client with separate receive thread for real-time messaging
- **Protocol**: Custom text-based protocol for communication
- **Storage**: File-based storage for messages and activity logs

## Building

### Windows (MinGW/MSVC)

```bash
# Using MinGW
make

# Or compile manually
gcc -Wall -Wextra -std=c11 -o server.exe server.c common.c -lws2_32
gcc -Wall -Wextra -std=c11 -o client.exe client.c common.c -lws2_32
```

### Linux

```bash
make

# Or compile manually
gcc -Wall -Wextra -std=c11 -o server server.c common.c -pthread
gcc -Wall -Wextra -std=c11 -o client client.c common.c -pthread
```

## Running

### Quick Start

1. **Build the project:**
   ```bash
   make
   ```

2. **Start the server** (in one terminal):
   ```bash
   # Windows
   server.exe
   
   # Linux
   ./server
   ```
   You should see: `Server started on port 8080`

3. **Start a client** (in another terminal):
   ```bash
   # Windows
   client.exe
   
   # Linux
   ./client
   ```
   To connect to a remote server: `client.exe 192.168.1.100`

4. **Use the menu** to register, login, and chat!

**See `QUICKSTART.md` for detailed step-by-step instructions and examples.**

## Usage

1. **Register**: Create a new account with username and password
2. **Login**: Login with your credentials
3. **Get Friends**: View your friend list and their online status
4. **Send Message**: Send 1-1 messages to other users
5. **Create Group**: Create a new group chat
6. **Group Operations**: Add/remove members, send group messages
7. **Search**: Search through chat history
8. **Block/Unblock**: Manage blocked users
9. **Pin Messages**: Pin important messages in groups

## Files

- `server.c` / `server.h`: Server implementation
- `client.c` / `client.h`: Client implementation
- `common.c` / `common.h`: Shared utilities and data structures
- `Makefile`: Build configuration
- `activity.log`: Activity log file (created at runtime)
- `messages.txt`: Message storage file (created at runtime)
 - `account.txt`: Simple username/password persistence (one account per line: `username password`). The server loads this file at startup and appends new accounts on successful registration.

## Protocol

The application uses a custom text-based protocol:
```
CMD:<command_type>|SENDER:<username>|RECIPIENT:<recipient>|CONTENT:<content>|EXTRA:<extra_data>|TYPE:<message_type>|PINNED:<0|1>|
```

## Notes

- The server supports multiple concurrent clients using multithreading
- Messages are stored in `messages.txt` for persistence
- Activity logs are written to `activity.log`
- Offline messages are stored and can be retrieved when users come online
- Group administrators can manage group members and settings
