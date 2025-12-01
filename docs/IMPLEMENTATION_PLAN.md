# Kế hoạch hoàn thiện Chat Application - TCP Multi-threaded Server/Client

## Tổng quan
Dự án hiện có cấu trúc cơ bản hoàn chỉnh nhưng thiếu một số tính năng quan trọng và cần tối ưu hóa. Kế hoạch này chia thành 3 phases với các task nhỏ, mỗi task có thể hoàn thành độc lập.

## Phân tích hiện trạng

### Đã có:
- ✅ Cấu trúc server/client multi-threaded
- ✅ Login/Register với file persistence (`account.txt`)
- ✅ Chat 1-1 và Group chat
- ✅ Activity logging (`activity.log`)
- ✅ Message saving (`messages.txt`)
- ✅ Search history function
- ✅ Cross-platform support (Windows/Linux)

### Cần bổ sung/tối ưu:
- ❌ **Single Session Enforcement**: Chưa check user đã online khi login
- ❌ **Offline Messages**: Chưa load và gửi tin nhắn offline khi user login
- ❌ **Stream Handling**: Chưa xử lý partial send/recv
- ❌ **Thread Safety**: File writes chưa có mutex protection
- ❌ **Pin Message Support**: Format file chưa hỗ trợ PINNED flag
- ⚠️ **Block User Logic**: Cần verify và cải thiện logic chặn
- ⚠️ **UI/UX**: Cần cải thiện hiển thị online/offline status và message display

---

## Phase 1: Core Refinement & Single Session (Giang's Focus)

### Task 1.1: Implement Single Session Enforcement Logic
**File**: `Chat_App/server.c` (function `handle_client`, case `CMD_LOGIN`)

**Mô tả**: 
- Khi user login, check `user->is_online` và `user->socket != INVALID_SOCKET`
- Nếu đã online:
  - Gửi message `CMD_RECEIVE_MESSAGE` với content "Your session was terminated due to new login" đến socket cũ
  - Đóng socket cũ (có thể dùng `shutdown()` trước `close_socket()`)
  - Log activity: "SESSION_TERMINATED" với details "Old session closed by new login"
- Sau đó mới set `user->is_online = true` và `user->socket = client_socket`

**Code location**: Lines 266-280 trong `server.c`

**Dependencies**: None

**Implementation Steps**:
1. Trong case `CMD_LOGIN`, trước khi set `user->is_online = true`, check:
   ```c
   if (user->is_online && user->socket != INVALID_SOCKET) {
       // Send termination message to old socket
       // Close old socket
       // Log activity
   }
   ```
2. Sau đó mới set `user->is_online = true` và `user->socket = client_socket`

---

### Task 1.2: Fix Stream Handling - Implement Reliable Send/Recv
**Files**: 
- `Chat_App/server.c` (function `send_response`)
- `Chat_App/common.c` (tạo helper functions)
- `Chat_App/common.h` (declare functions)

**Mô tả**:
- Tạo function `send_all(socket_t sock, const char* buf, size_t len)` trong `common.c`:
  - Loop gửi cho đến khi gửi hết data (handle partial sends)
  - Return số bytes đã gửi hoặc -1 nếu error
- Tạo function `recv_all(socket_t sock, char* buf, size_t len)` trong `common.c`:
  - Loop nhận cho đến khi nhận đủ data hoặc connection closed
  - Return số bytes đã nhận
- Update `send_response()` trong `server.c` để dùng `send_all()` thay vì `send()`
- Update `receive_response()` trong `client.c` để dùng `recv_all()` nếu cần

**Code locations**: 
- `server.c` line 183-202 (send_response)
- `client.c` line 89-130 (receive_response)
- Tạo mới trong `common.c` và declare trong `common.h`

**Dependencies**: None

**Implementation Steps**:
1. Thêm vào `common.h`:
   ```c
   int send_all(socket_t sock, const char* buf, size_t len);
   int recv_all(socket_t sock, char* buf, size_t len);
   ```
2. Implement trong `common.c`:
   ```c
   int send_all(socket_t sock, const char* buf, size_t len) {
       size_t total_sent = 0;
       while (total_sent < len) {
           int sent = send(sock, buf + total_sent, len - total_sent, 0);
           if (sent == SOCKET_ERROR) return -1;
           total_sent += sent;
       }
       return total_sent;
   }
   ```
3. Update `send_response()` để dùng `send_all()` thay vì `send()`

---

### Task 1.3: Enhance Activity Logging Format with Thread Safety
**File**: `Chat_App/common.c` (function `log_activity`)

**Mô tả**:
- Đảm bảo format log chuẩn: `[TIMESTAMP] User: USERNAME | Action: ACTION | Details: DETAILS`
- Thêm logging cho các events quan trọng:
  - SESSION_TERMINATED (khi đá session cũ)
  - LOGOUT (khi user disconnect)
  - LAST_SEEN (có thể dùng để track "lần cuối online")
- **CRITICAL - Thread Safety**: 
  - Tạo global mutex cho file logging trong `common.c` hoặc `common.h`:
    ```c
    #ifdef _WIN32
    static CRITICAL_SECTION log_mutex;
    static bool log_mutex_initialized = false;
    #else
    static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
    #endif
    ```
  - Initialize mutex trong function `init_log_mutex()` (gọi từ `init_server()`)
  - Lock mutex trước khi `fopen()` và unlock sau khi `fclose()` trong `log_activity()`
  - Đảm bảo atomic write để tránh corruption khi nhiều threads ghi cùng lúc

**Code location**: `common.c` lines 3-14

**Dependencies**: Task 1.1 (để log SESSION_TERMINATED)

**Implementation Steps**:
1. Thêm mutex declaration vào `common.c` (static, file-level)
2. Tạo function `init_log_mutex()`:
   ```c
   void init_log_mutex(void) {
       #ifdef _WIN32
       if (!log_mutex_initialized) {
           InitializeCriticalSection(&log_mutex);
           log_mutex_initialized = true;
       }
       #else
       // Already initialized with PTHREAD_MUTEX_INITIALIZER
       #endif
   }
   ```
3. Gọi `init_log_mutex()` trong `init_server()` (server.c line 128-133)
4. Update `log_activity()`:
   ```c
   void log_activity(const char* username, const char* action, const char* details) {
       #ifdef _WIN32
       EnterCriticalSection(&log_mutex);
       #else
       pthread_mutex_lock(&log_mutex);
       #endif
       
       FILE* log_file = fopen("activity.log", "a");
       if (log_file) {
           // ... existing code ...
           fclose(log_file);
       }
       
       #ifdef _WIN32
       LeaveCriticalSection(&log_mutex);
       #else
       pthread_mutex_unlock(&log_mutex);
       #endif
   }
   ```

---

## Phase 2: Offline Messages & File Handling (Ha's & Shared Focus)

### Task 2.1: Design Messages.txt Format with Pin Support
**File**: `Chat_App/server.c` (function `save_message_to_file`)

**Mô tả**:
- Quyết định format cho `messages.txt`:
  - **Format chuẩn**: `TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED`
  - Ví dụ: `2024-01-15 10:30:45|alice|bob|0|Hello|1|0`
  - `TYPE`: 0=MSG_TEXT, 1=MSG_EMOJI, 2=MSG_SYSTEM
  - `DELIVERED`: 0=chưa gửi (offline), 1=đã gửi (online)
  - `PINNED`: 0=không ghim, 1=đã ghim
- Update function `save_message_to_file()` trong `server.c` để:
  - Thêm parameters: `bool recipient_online`, `bool is_pinned`
  - Ghi theo format mới với các flags tương ứng

**Code location**: `server.c` lines 846-856

**Dependencies**: None

**Implementation Steps**:
1. Update function signature:
   ```c
   void save_message_to_file(const char* sender, const char* recipient, 
                            const char* content, bool is_group, 
                            bool recipient_online, bool is_pinned)
   ```
2. Update format string:
   ```c
   fprintf(file, "%s|%s|%s|%d|%s|%d|%d\n", 
           time_str, sender, recipient, 
           is_group ? 2 : 0, // TYPE: 0=1-1, 2=GROUP
           content, 
           recipient_online ? 1 : 0, // DELIVERED
           is_pinned ? 1 : 0); // PINNED
   ```

---

### Task 2.2: Implement Load Offline Messages Function
**File**: `Chat_App/server.c` (tạo function mới)

**Mô tả**:
- Tạo function `load_and_send_offline_messages(socket_t socket, const char* username)`:
  - Đọc `messages.txt` line by line
  - Parse các dòng có format `TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED`
  - Filter: `RECIPIENT == username` và `DELIVERED == 0`
  - Gửi từng message đến socket với format: `[TIMESTAMP] From SENDER: CONTENT`
  - Sau khi gửi thành công, mark message là `DELIVERED:1` (có thể update file hoặc để sau)
- Call function này trong case `CMD_LOGIN` sau khi login thành công
- **Thread Safety**: Sử dụng mutex khi đọc/ghi file (có thể dùng mutex từ Task 1.3 hoặc tạo riêng cho messages.txt)

**Code location**: 
- Tạo function mới sau `save_message_to_file()` (line 856)
- Call trong `handle_client()` case `CMD_LOGIN` (line 275-276)

**Dependencies**: Task 2.1 (format design)

**Implementation Steps**:
1. Tạo mutex cho messages.txt (tương tự log_mutex):
   ```c
   #ifdef _WIN32
   static CRITICAL_SECTION messages_mutex;
   static bool messages_mutex_initialized = false;
   #else
   static pthread_mutex_t messages_mutex = PTHREAD_MUTEX_INITIALIZER;
   #endif
   ```
2. Initialize trong `init_server()`
3. Implement function:
   ```c
   void load_and_send_offline_messages(socket_t socket, const char* username) {
       // Lock mutex
       FILE* file = fopen("messages.txt", "r");
       if (!file) return;
       
       char line[BUFFER_SIZE];
       while (fgets(line, sizeof(line), file)) {
           // Parse: TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED
           // Check if RECIPIENT == username && DELIVERED == 0
           // Send message to socket
           // Mark as delivered (update file or in-memory)
       }
       fclose(file);
       // Unlock mutex
   }
   ```
4. Call trong `CMD_LOGIN` case sau line 272

---

### Task 2.3: Update Message Saving with Thread Safety and Pin Support
**File**: `Chat_App/server.c` (function `save_message_to_file`)

**Mô tả**:
- Update `save_message_to_file()` signature:
  ```c
  void save_message_to_file(const char* sender, const char* recipient, 
                            const char* content, bool is_group, 
                            bool recipient_online, bool is_pinned)
  ```
- Ghi theo format mới: `TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED`
- **CRITICAL - Thread Safety**: 
  - Tạo mutex cho messages.txt (tương tự Task 1.3)
  - Lock mutex trước khi `fopen()` và unlock sau khi `fclose()`
  - Đảm bảo atomic write
- Update tất cả calls đến `save_message_to_file()`:
  - `CMD_SEND_MESSAGE` (line 375): Check `recipient->is_online`, pass `msg->is_pinned`
  - `CMD_GROUP_MESSAGE` (line 611): Check từng member online status, pass `msg->is_pinned`

**Code locations**: 
- `server.c` line 846-856 (function definition)
- `server.c` line 375 (CMD_SEND_MESSAGE)
- `server.c` line 611 (CMD_GROUP_MESSAGE)

**Dependencies**: Task 2.1, Task 1.3 (mutex pattern)

**Implementation Steps**:
1. Update function signature và implementation với mutex
2. Update call ở line 375:
   ```c
   save_message_to_file(current_user->username, msg->recipient, msg->content, 
                       false, recipient->is_online, msg->is_pinned);
   ```
3. Update call ở line 611:
   ```c
   // For each member, check if online
   bool member_online = (member && member->is_online && member->socket != INVALID_SOCKET);
   save_message_to_file(current_user->username, msg->recipient, msg->content, 
                       true, member_online, msg->is_pinned);
   ```

---

### Task 2.4: Optimize Search History Function
**File**: `Chat_App/server.c` (function `search_messages`)

**Mô tả**:
- Kiểm tra function `search_messages()` hiện tại (lines 859-880)
- Update để parse format mới từ Task 2.1:
  - Parse `TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED`
  - Filter theo username và recipient đúng cách
  - Return kết quả có format đẹp: `[TIMESTAMP] SENDER -> RECIPIENT: CONTENT [PINNED]` (nếu pinned)
- Test với file lớn (nếu cần, có thể limit số dòng đọc)

**Code location**: `server.c` lines 859-880

**Dependencies**: Task 2.1

**Implementation Steps**:
1. Update parsing logic:
   ```c
   char* token = strtok(line, "|");
   // token[0] = TIMESTAMP
   // token[1] = SENDER
   // token[2] = RECIPIENT
   // token[3] = TYPE
   // token[4] = CONTENT
   // token[5] = DELIVERED
   // token[6] = PINNED
   ```
2. Check if message matches username and recipient
3. Format result string with PINNED flag if needed

---

### Task 2.5: Verify and Improve Block User Logic
**File**: `Chat_App/server.c` (case `CMD_SEND_MESSAGE`)

**Mô tả**:
- **Verify logic hiện tại**: Check line 359 - logic hiện tại:
  ```c
  if (is_blocked(current_user, msg->recipient) || is_blocked(recipient, current_user->username))
  ```
  - Logic này check cả 2 chiều (A block B hoặc B block A) -> đều từ chối
- **Yêu cầu mới**: Chỉ từ chối nếu **Recipient block Sender** (không phải ngược lại)
- **Fix logic**:
  - Chỉ check `is_blocked(recipient, current_user->username)` 
  - Nếu recipient block sender -> từ chối với message "You are blocked by this user"
  - Nếu sender block recipient -> có thể vẫn cho phép gửi (hoặc từ chối tùy yêu cầu, nhưng thường là cho phép)
- **Update error message** để rõ ràng hơn: "You are blocked by this user" thay vì "User is blocked"
- **Test**: Verify với các scenarios:
  - A block B, B gửi cho A -> từ chối
  - A block B, A gửi cho B -> có thể cho phép (hoặc từ chối tùy logic)

**Code location**: `server.c` lines 347-401 (CMD_SEND_MESSAGE case)

**Dependencies**: None

**Implementation Steps**:
1. Update line 359:
   ```c
   // Only check if recipient blocked sender
   if (is_blocked(recipient, current_user->username)) {
       send_response(client_socket, CMD_ERROR, "You are blocked by this user");
       break;
   }
   ```
2. Remove check `is_blocked(current_user, msg->recipient)` (hoặc giữ lại nếu muốn từ chối cả 2 chiều)

---

## Phase 3: UI/UX & Connectivity (Integration)

### Task 3.1: Improve Client UI - Separate Input/Output Threads
**File**: `Chat_App/client.c`

**Mô tả**:
- Hiện tại có `receive_thread()` nhưng UI vẫn bị interrupt khi message đến
- Cải thiện:
  - Sử dụng mutex hoặc queue để buffer messages từ receive thread
  - Display messages với format: `\n[HH:MM:SS] SENDER: CONTENT\n> ` (preserve prompt)
  - Sử dụng `fflush(stdout)` đúng chỗ
  - Có thể dùng ANSI escape codes để clear/redraw nếu cần (optional)

**Code locations**: 
- `client.c` lines 103-130 (receive_response)
- `client.c` lines 133-139 (receive_thread)

**Dependencies**: None

**Implementation Steps**:
1. Update `receive_response()` để format message đẹp hơn:
   ```c
   case CMD_RECEIVE_MESSAGE: {
       time_t now = time(NULL);
       struct tm* timeinfo = localtime(&now);
       char time_str[20];
       strftime(time_str, 20, "%H:%M:%S", timeinfo);
       printf("\n[%s] %s: %s\n> ", time_str, msg->sender, msg->content);
       fflush(stdout);
       break;
   }
   ```

---

### Task 3.2: Enhance Friends List Display with Online/Offline Status
**File**: `Chat_App/server.c` (case `CMD_GET_FRIENDS`)

**Mô tả**:
- Cải thiện format output của friend list:
  - Format: `Friends List:\n  - USERNAME1 [ONLINE]\n  - USERNAME2 [OFFLINE]\n  - USERNAME3 [ONLINE]`
  - Hoặc: `USERNAME1 (online) | USERNAME2 (offline) | USERNAME3 (online)`
- Có thể thêm "Last seen" từ `activity.log` nếu có LOGOUT event (optional, nếu có thời gian)

**Code location**: `server.c` lines 300-317

**Dependencies**: Task 1.3 (enhanced logging)

**Implementation Steps**:
1. Update line 306-314:
   ```c
   char friend_list[BUFFER_SIZE] = "Friends List:\n";
   for (int i = 0; i < current_user->friend_count; i++) {
       User* friend = find_user(state, current_user->friends[i]);
       if (friend) {
           char status[20];
           strcpy(status, friend->is_online ? "[ONLINE]" : "[OFFLINE]");
           char line[200];
           snprintf(line, sizeof(line), "  - %s %s\n", friend->username, status);
           strcat(friend_list, line);
       }
   }
   ```

---

### Task 3.3: Fix Client Sleep Bug
**File**: `Chat_App/client.c` (function `handle_user_input`)

**Mô tả**:
- Line 401 có bug: `sleep(100000)` trên Linux (100 giây!) - nên là `usleep(100000)` (100ms)
- Fix: Dùng `#ifdef _WIN32 Sleep(100) #else usleep(100000) #endif`
- Hoặc tốt hơn: Remove sleep này vì receive thread đã xử lý responses

**Code location**: `client.c` lines 397-403

**Dependencies**: None

**Implementation Steps**:
1. Replace line 401:
   ```c
   #ifdef _WIN32
   Sleep(100);
   #else
   usleep(100000);  // 100ms
   #endif
   ```
   Hoặc remove hoàn toàn nếu không cần thiết.

---

### Task 3.4: Manual Task - Network Configuration Guide
**File**: Tạo `Chat_App/NETWORK_SETUP.md`

**Mô tả** (Manual task cho User):
- Hướng dẫn lấy IP server:
  - Windows: `ipconfig` → tìm IPv4 Address (ví dụ: 192.168.1.100)
  - Linux: `ifconfig` hoặc `ip addr` → tìm inet address
- Hướng dẫn ping test:
  - `ping <server_ip>` từ client machine
- Hướng dẫn Firewall:
  - Windows: Mở port 8080 trong Windows Firewall
  - Linux: `sudo ufw allow 8080` hoặc `sudo iptables -A INPUT -p tcp --dport 8080 -j ACCEPT`
- Hướng dẫn VM Network:
  - Bridged: VM có IP riêng trong LAN
  - NAT: Cần port forwarding (phức tạp hơn)
- Verify server binding:
  - Server đã bind `INADDR_ANY` (0.0.0.0) nên OK
  - Chỉ cần đảm bảo firewall cho phép

**Dependencies**: None (Manual task)

**Implementation Steps**:
1. Tạo file `NETWORK_SETUP.md` với nội dung hướng dẫn chi tiết

---

## Phase 4: Testing & Validation (Optional but Recommended)

### Task 4.1: Test Single Session Enforcement
**Mô tả**: 
- Test case: Login user A từ client 1, login lại user A từ client 2
- Expected: Client 1 nhận thông báo "session terminated", client 2 login thành công

**Dependencies**: Task 1.1

---

### Task 4.2: Test Offline Messages
**Mô tả**:
- Test case: User A gửi message cho User B (B offline), B login sau đó
- Expected: B nhận message ngay khi login

**Dependencies**: Task 2.2, Task 2.3

---

### Task 4.3: Test Thread Safety
**Mô tả**:
- Test case: Nhiều users gửi message cùng lúc
- Expected: File `messages.txt` và `activity.log` không bị corruption, tất cả messages được ghi đúng

**Dependencies**: Task 1.3, Task 2.3

---

### Task 4.4: Test Block User Logic
**Mô tả**:
- Test case: User A block User B, B cố gửi message cho A
- Expected: B nhận error "You are blocked by this user"

**Dependencies**: Task 2.5

---

### Task 4.5: Test Cross-Machine Connection
**Mô tả**:
- Test server trên máy A, client trên máy B (hoặc VM)
- Verify connection và chat hoạt động

**Dependencies**: Task 3.4 (network setup)

---

## File Structure Reference

- `Chat_App/server.c` - Server implementation (932 lines)
- `Chat_App/client.c` - Client implementation (443 lines)  
- `Chat_App/common.c` - Shared utilities (78 lines)
- `Chat_App/server.h` - Server headers
- `Chat_App/client.h` - Client headers
- `Chat_App/common.h` - Common definitions (140 lines)
- `Chat_App/account.txt` - Account storage
- `Chat_App/messages.txt` - Message storage (created at runtime)
- `Chat_App/activity.log` - Activity log (created at runtime)

## Code Locations Summary

### Server.c Key Locations:
- Line 65-145: `init_server()` - Initialize server
- Line 148-155: `find_user()` - Find user by username
- Line 183-202: `send_response()` - Send response to client
- Line 234-812: `handle_client()` - Main client handler thread
  - Line 266-280: `CMD_LOGIN` case - **Task 1.1**
  - Line 300-317: `CMD_GET_FRIENDS` case - **Task 3.2**
  - Line 347-401: `CMD_SEND_MESSAGE` case - **Task 2.3, Task 2.5**
  - Line 575-643: `CMD_GROUP_MESSAGE` case - **Task 2.3**
  - Line 645-665: `CMD_SEARCH_HISTORY` case - **Task 2.4**
- Line 846-856: `save_message_to_file()` - **Task 2.1, Task 2.3**
- Line 859-880: `search_messages()` - **Task 2.4**

### Client.c Key Locations:
- Line 89-130: `receive_response()` - **Task 3.1**
- Line 133-139: `receive_thread()` - **Task 3.1**
- Line 397-403: `handle_user_input()` sleep bug - **Task 3.3**

### Common.c Key Locations:
- Line 3-14: `log_activity()` - **Task 1.3**
- New functions needed: `send_all()`, `recv_all()` - **Task 1.2**

## Dependencies Graph

```
Task 1.1 (Single Session) → Task 1.3 (Log SESSION_TERMINATED)
Task 1.2 (Stream Handling) → None
Task 1.3 (Thread Safety Log) → Task 1.1

Task 2.1 (Format Design) → None
Task 2.2 (Load Offline) → Task 2.1
Task 2.3 (Save Message) → Task 2.1, Task 1.3 (mutex pattern)
Task 2.4 (Search) → Task 2.1
Task 2.5 (Block Logic) → None

Task 3.1 (UI Improvement) → None
Task 3.2 (Friends List) → Task 1.3
Task 3.3 (Sleep Bug) → None
Task 3.4 (Network Guide) → None (Manual)
```

## Notes

- Tất cả tasks có thể làm song song nếu không có dependencies
- Priority: Phase 1 > Phase 2 > Phase 3
- Task 3.4 là manual task, không cần code
- Testing có thể làm sau khi hoàn thành các phases chính
- **Thread Safety là CRITICAL** - không được bỏ qua mutex cho file operations
- Format file mới: `TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED`
- Block logic: Chỉ check recipient block sender (không phải ngược lại)

## Implementation Order Recommendation

1. **Phase 1** (Core):
   - Task 1.1 → Task 1.3 (có dependency)
   - Task 1.2 (độc lập, có thể làm song song)

2. **Phase 2** (Features):
   - Task 2.1 → Task 2.2, Task 2.3, Task 2.4 (có dependency)
   - Task 2.5 (độc lập)

3. **Phase 3** (UI/UX):
   - Tất cả tasks độc lập, có thể làm song song

4. **Phase 4** (Testing):
   - Sau khi hoàn thành các phases chính

## Critical Reminders

1. **Mutex cho File Operations**: 
   - `log_activity()` cần mutex
   - `save_message_to_file()` cần mutex
   - `load_and_send_offline_messages()` cần mutex

2. **Format Consistency**:
   - Tất cả messages phải theo format: `TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED`
   - Parse đúng format khi đọc file

3. **Error Handling**:
   - Luôn check return values của file operations
   - Handle errors gracefully với proper error messages

4. **Cross-platform**:
   - Sử dụng `#ifdef _WIN32` cho Windows-specific code
   - Sử dụng POSIX cho Linux code





