# TÓM TẮT 3 TÍNH NĂNG MỚI

## 1. Friend Request Flow ✅

### Thay đổi:
- **Trước**: Add friend trực tiếp (CMD_ADD_FRIEND)
- **Sau**: Luồng Request -> Accept

### Commands mới:
- `CMD_SEND_FRIEND_REQUEST` (19): Gửi friend request
- `CMD_ACCEPT_FRIEND_REQUEST` (20): Chấp nhận request
- `CMD_GET_FRIEND_REQUESTS` (21): Xem danh sách requests

### Cách sử dụng:
1. **Gửi request**: Menu option 18 → Nhập username
2. **Xem requests**: Menu option 19
3. **Chấp nhận**: Menu option 20 → Nhập username

### Implementation:
- Thêm `pending_requests[]` và `pending_request_count` vào User struct
- Server tự động notify user khi có request mới (nếu online)
- Server tự động notify khi request được accept

### Files thay đổi:
- `common.h`: Thêm commands và fields vào User struct
- `server.c`: Thêm 3 case handlers mới
- `client.c`: Thêm menu options 18, 19, 20

---

## 2. Last Seen Feature ✅

### Thay đổi:
- **Trước**: `UserB [OFFLINE]`
- **Sau**: `UserB [OFFLINE - Last seen: 14:30]`

### Implementation:
- Thêm `last_seen` (time_t) vào User struct
- Update `last_seen = time(NULL)` khi user logout/disconnect
- Parse `last_seen` trong `CMD_GET_FRIENDS` để hiển thị

### Format hiển thị:
- **Online**: `UserB [ONLINE]`
- **Offline có last_seen**: `UserB [OFFLINE - Last seen: 14:30]`
- **Offline chưa có last_seen**: `UserB [OFFLINE]`

### Files thay đổi:
- `common.h`: Thêm `last_seen` vào User struct
- `server.c`: 
  - Update `last_seen` khi logout (line ~896)
  - Update `CMD_GET_FRIENDS` để hiển thị last_seen (line ~330-350)
  - Khởi tạo `last_seen = 0` trong `add_user()`

---

## 3. Emoji Support & Sound Notification ✅

### Emoji Support:
- Thêm `SetConsoleOutputCP(65001);` vào đầu `main()` trong `client.c`
- Chỉ áp dụng cho Windows (Linux đã hỗ trợ UTF-8 mặc định)

### Sound Notification:
- Thêm `printf("\a");` khi nhận `CMD_RECEIVE_MESSAGE`
- `\a` là System Bell character, tạo tiếng beep

### Implementation:
```c
// In client.c main():
#ifdef _WIN32
SetConsoleOutputCP(65001);  // UTF-8 support
#endif

// In receive_response(), case CMD_RECEIVE_MESSAGE:
printf("\a");  // Sound notification
```

### Files thay đổi:
- `client.c`: 
  - Thêm `SetConsoleOutputCP(65001)` trong `main()`
  - Thêm `printf("\a")` trong `receive_response()`

---

## TÓM TẮT THAY ĐỔI CODE

### common.h:
```c
// Thêm commands:
CMD_SEND_FRIEND_REQUEST = 19,
CMD_ACCEPT_FRIEND_REQUEST = 20,
CMD_GET_FRIEND_REQUESTS = 21,

// Thêm vào User struct:
char pending_requests[MAX_FRIENDS][MAX_USERNAME];
int pending_request_count;
time_t last_seen;
```

### server.c:
- Thêm 3 case handlers: `CMD_SEND_FRIEND_REQUEST`, `CMD_ACCEPT_FRIEND_REQUEST`, `CMD_GET_FRIEND_REQUESTS`
- Update `CMD_GET_FRIENDS` để hiển thị last_seen
- Update logout để set `last_seen = time(NULL)`
- Update `add_user()` để khởi tạo fields mới

### client.c:
- Thêm `SetConsoleOutputCP(65001)` trong `main()`
- Thêm `printf("\a")` trong `receive_response()`
- Thêm menu options 18, 19, 20
- Thêm case `CMD_GET_FRIEND_REQUESTS` trong `receive_response()`

---

## TESTING

### Test Friend Request:
1. UserA: Menu 18 → Send request to UserB
2. UserB: Menu 19 → Xem requests (sẽ thấy UserA)
3. UserB: Menu 20 → Accept request from UserA
4. UserA và UserB: Menu 3 → Check friends list (sẽ thấy nhau)

### Test Last Seen:
1. UserA login → UserB: Menu 3 → Thấy `UserA [ONLINE]`
2. UserA logout/disconnect
3. UserB: Menu 3 → Thấy `UserA [OFFLINE - Last seen: HH:MM]`

### Test Sound & Emoji:
1. UserA gửi message cho UserB
2. UserB sẽ nghe tiếng beep (`\a`)
3. Emoji sẽ hiển thị đúng nếu dùng UTF-8 emoji

---

## LƯU Ý

1. **Friend Request**: CMD_ADD_FRIEND vẫn hoạt động (backward compatibility)
2. **Last Seen**: Chỉ hiển thị nếu user đã từng logout (last_seen > 0)
3. **Sound**: Chỉ hoạt động trên terminal hỗ trợ System Bell
4. **Emoji**: Chỉ Windows cần SetConsoleOutputCP, Linux đã hỗ trợ sẵn

---

**Ngày implement**: 2025-11-30  
**Status**: ✅ Hoàn thành




