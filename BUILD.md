# Build Instructions

## Windows (MinGW/MSVC)

### Using MinGW (Recommended)

1. Install MinGW-w64 or use MSYS2
2. Open terminal in project directory
3. Run:
   ```bash
   make
   ```
   Or manually:
   ```bash
   gcc -Wall -Wextra -std=c11 -o server.exe server.c common.c -lws2_32
   gcc -Wall -Wextra -std=c11 -o client.exe client.c common.c -lws2_32
   ```

### Using Visual Studio

1. Create a new C project
2. Add all .c and .h files
3. Link against ws2_32.lib
4. Build the project

## Linux

1. Install build tools:
   ```bash
   sudo apt-get install build-essential  # Ubuntu/Debian
   # or
   sudo yum install gcc make            # CentOS/RHEL
   ```

2. Build:
   ```bash
   make
   ```
   Or manually:
   ```bash
   gcc -Wall -Wextra -std=c11 -o server server.c common.c -pthread
   gcc -Wall -Wextra -std=c11 -o client client.c common.c -pthread
   ```

## Running

1. Start the server:
   ```bash
   # Windows
   server.exe
   
   # Linux
   ./server
   ```

2. Start clients in separate terminals:
   ```bash
   # Windows
   client.exe
   
   # Linux
   ./client
   ```

## Testing

1. Register users:
   - Option 1: Register
   - Enter username and password

2. Login:
   - Option 2: Login
   - Enter credentials

3. Add friends:
   - Option 4: Add Friend
   - Enter friend's username

4. Send messages:
   - Option 5: Send Message
   - Enter recipient and message

5. Create groups:
   - Option 6: Create Group
   - Enter group name

6. Test other features as needed

