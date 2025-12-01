# Quick Start Guide

## Step-by-Step Instructions

### Step 1: Build the Project

#### On Windows:

**Option A: Using Make (if you have make installed)**
```bash
make
```

**Option B: Manual Compilation**
```bash
gcc -Wall -Wextra -std=c11 -o server.exe server.c common.c -lws2_32
gcc -Wall -Wextra -std=c11 -o client.exe client.c common.c -lws2_32
```

#### On Linux:

**Option A: Using Make**
```bash
make
```

**Option B: Manual Compilation**
```bash
gcc -Wall -Wextra -std=c11 -o server server.c common.c -pthread
gcc -Wall -Wextra -std=c11 -o client client.c common.c -pthread
```

---

### Step 2: Run the Server

Open a terminal/command prompt and run:

**Windows:**
```bash
server.exe
```

**Linux:**
```bash
./server
```

You should see:
```
Server started on port 8080
Waiting for clients...
```

**Keep this terminal open!** The server must be running for clients to connect.

---

### Step 3: Run the Client(s)

Open **one or more new terminals** (keep the server running) and run:

**Windows:**
```bash
client.exe
```
or to connect to a remote server:
```bash
client.exe 192.168.1.100
```

**Linux:**
```bash
./client
```
or to connect to a remote server:
```bash
./client 192.168.1.100
```

---

### Step 4: Use the Application

When you run the client, you'll see a menu:

```
=== Chat Application Menu ===
1. Register
2. Login
3. Get Friends List
4. Add Friend
5. Send Message (1-1)
6. Create Group
7. Add User to Group
8. Remove User from Group
9. Leave Group
10. Send Group Message
11. Search Chat History
12. Set Group Name
13. Block User
14. Unblock User
15. Pin Message
16. Get Pinned Messages
17. Disconnect
0. Exit
Choice:
```

#### Example Workflow:

1. **Register a new user:**
   - Choose option `1`
   - Enter username: `alice`
   - Enter password: `password123`

2. **Login:**
   - Choose option `2`
   - Enter username: `alice`
   - Enter password: `password123`

3. **Register another user (in a different client terminal):**
   - Choose option `1`
   - Enter username: `bob`
   - Enter password: `password456`

4. **Login as bob:**
   - Choose option `2`
   - Enter username: `bob`
   - Enter password: `password456`

5. **Add friend:**
   - As `alice`, choose option `4`
   - Enter friend username: `bob`
   - As `bob`, choose option `4`
   - Enter friend username: `alice`

6. **Send a message:**
   - As `alice`, choose option `5`
   - Enter recipient: `bob`
   - Enter message: `Hello Bob!`
   - Bob will receive the message in real-time!

7. **Create a group:**
   - As `alice`, choose option `6`
   - Enter group name: `My Group`

8. **Add user to group:**
   - As `alice`, choose option `7`
   - Enter group ID: `GROUP_alice_1234567890` (shown when group was created)
   - Enter username to add: `bob`

9. **Send group message:**
   - As `alice`, choose option `10`
   - Enter group ID: `GROUP_alice_1234567890`
   - Enter message: `Hello everyone!`
   - All group members will receive it!

---

## Troubleshooting

### "Socket creation failed"
- Make sure no other application is using port 8080
- On Windows, ensure Winsock is properly initialized
- Try running as administrator (Windows)

### "Connection failed"
- Make sure the server is running
- Check if firewall is blocking the connection
- Verify the server IP address is correct

### "Bind failed"
- Port 8080 might be in use
- Try changing the PORT in `common.h` (line 49)
- On Linux, you might need sudo for ports < 1024

### Compilation Errors
- **Windows**: Make sure you have MinGW or MSVC installed
- **Linux**: Install build-essential: `sudo apt-get install build-essential`
- Make sure all `.c` and `.h` files are in the same directory

---

## Testing with Multiple Clients

To test the full functionality:

1. Start the server (one terminal)
2. Start client 1 (second terminal) - Register as `user1`
3. Start client 2 (third terminal) - Register as `user2`
4. Start client 3 (fourth terminal) - Register as `user3`

Now you can:
- Add friends between users
- Send 1-1 messages
- Create groups and add multiple users
- Test group messaging
- Test all features!

---

## Files Created at Runtime

When you run the application, these files will be created:
- `activity.log` - Logs all user activities
- `messages.txt` - Stores all messages (for offline delivery and search)

You can view these files to see the activity and message history.

