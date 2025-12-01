# Hướng Dẫn Test Toàn Diện - Chat Application

## Mục Lục

1. [Chuẩn Bị Môi Trường](#chuẩn-bị-môi-trường)
2. [Test Cơ Bản (Build & Run)](#test-cơ-bản)
3. [Test Chức Năng Cốt Lõi](#test-chức-năng-cốt-lõi)
4. [Test Edge Cases](#test-edge-cases)
5. [Test Persistence](#test-persistence)
6. [Test UI/UX](#test-uiux)
7. [Test Tự Động](#test-tự-động)
8. [Checklist Tổng Hợp](#checklist-tổng-hợp)

---

## 1. Chuẩn Bị Môi Trường

### Yêu Cầu:
- Windows 10+ hoặc Linux
- GCC compiler (MinGW trên Windows)
- 2 terminal windows (hoặc 2 máy tính)
- Windows Terminal hoặc PowerShell (khuyến nghị cho Windows)

### Bước 1: Kiểm Tra Cấu Trúc Thư Mục

```powershell
# Kiểm tra các thư mục cần thiết
Test-Path "src"
Test-Path "include"
Test-Path "data"
Test-Path "logs"
Test-Path "build"
```

**Kết quả mong đợi:** Tất cả trả về `True`

### Bước 2: Tạo Thư Mục Nếu Thiếu

```powershell
# Tạo thư mục nếu chưa có
New-Item -ItemType Directory -Path "data" -Force | Out-Null
New-Item -ItemType Directory -Path "logs" -Force | Out-Null
New-Item -ItemType Directory -Path "build" -Force | Out-Null
```

---

## 2. Test Cơ Bản (Build & Run)

### Test 2.1: Compile Project

```powershell
# Cách 1: Dùng Makefile (nếu có make)
make clean
make

# Cách 2: Compile thủ công
gcc -Wall -Wextra -std=c11 -Iinclude -c src/common.c -o build/common.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/server.c -o build/server.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/client.c -o build/client.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/ui.c -o build/ui.o

# Link server
gcc -Wall -Wextra -std=c11 -Iinclude -o build/server.exe build/server.o build/common.o -lws2_32

# Link client
gcc -Wall -Wextra -std=c11 -Iinclude -o build/client.exe build/client.o build/common.o build/ui.o -lws2_32
```

**Kết quả mong đợi:**
- ✅ Compile thành công, không có warning/error
- ✅ Tạo được `build/server.exe` và `build/client.exe`

### Test 2.2: Chạy Server

**Terminal 1:**
```powershell
cd build
.\server.exe
```

**Kết quả mong đợi:**
```
Server started on port 8080
Waiting for clients...
```

**Kiểm tra:**
- ✅ Server khởi động không lỗi
- ✅ Port 8080 đang listen
- ✅ File `../data/account.txt` được tạo (nếu chưa có)
- ✅ File `../logs/activity.log` được tạo

### Test 2.3: Chạy Client

**Terminal 2:**
```powershell
cd build
.\client.exe
```

**Kết quả mong đợi:**
- ✅ Client khởi động
- ✅ Hiển thị menu chính
- ✅ Có thể nhập lệnh

---

## 3. Test Chức Năng Cốt Lõi

### Test 3.1: Đăng Ký Tài Khoản

**Client Terminal:**
```
1  (Chọn Register)
Nhập username: testuser1
Nhập password: password123
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✓] Registration successful`
- ✅ File `../data/account.txt` có dòng mới: `testuser1 password123`

**Lặp lại với user thứ 2:**
```
1
testuser2
password456
```

### Test 3.2: Đăng Nhập

**Client Terminal:**
```
2  (Chọn Login)
Nhập username: testuser1
Nhập password: password123
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✓] Login successful`
- ✅ Menu chính hiển thị
- ✅ File `../logs/activity.log` có entry: `LOGIN | testuser1`

### Test 3.3: Kết Bạn

**Client 1 (testuser1):**
```
3  (Chọn Get Friends List)
4  (Chọn Add Friend)
Nhập username: testuser2
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✓] Friend request sent` hoặc `[✓] Friend added`
- ✅ File `../logs/activity.log` có entry: `ADD_FRIEND | testuser2`

**Client 2 (testuser2):**
- ✅ Nhận được thông báo friend request (nếu dùng request system)
- ✅ Hoặc tự động có trong friends list

### Test 3.4: Chat 1-1

**Client 1 (testuser1):**
```
11  (Chọn Chat 1-1)
Nhập username: testuser2
Nhập tin nhắn: Hello from user1!
```

**Client 2 (testuser2):**
- ✅ Nhận được tin nhắn ngay lập tức
- ✅ Hiển thị bong bóng chat với màu sắc đúng
- ✅ Có timestamp

**Client 2 gửi lại:**
```
11
testuser1
Hi user1, received!
```

**Client 1:**
- ✅ Nhận được tin nhắn
- ✅ Bong bóng căn đúng (ME: phải, FRIEND: trái)

### Test 3.5: Load Lịch Sử Chat

**Client 1:**
```
11  (Chọn Chat 1-1)
testuser2
```

**Kết quả mong đợi:**
- ✅ **QUAN TRỌNG:** Lịch sử tin nhắn cũ hiển thị NGAY khi vào phòng chat
- ✅ Không phải màn hình trống
- ✅ Tất cả tin nhắn 2 chiều (user1→user2 và user2→user1) đều hiển thị

### Test 3.6: Group Chat

**Client 1:**
```
7  (Chọn Create Group)
Nhập tên nhóm: Test Group
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `Group created: GROUP_...`
- ✅ Lưu lại Group ID

**Client 1:**
```
8  (Chọn Add Member to Group)
Nhập Group ID: GROUP_... (từ bước trước)
Nhập username: testuser2
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✓] User added to group`

**Client 1:**
```
12  (Chọn Group Chat)
Nhập Group ID: GROUP_...
Nhập tin nhắn: Group message test
```

**Client 2:**
- ✅ Nhận được tin nhắn group
- ✅ Hiển thị đúng format group message

---

## 4. Test Edge Cases

### Test 4.1: Self-Unfriend

**Client 1:**
```
9  (Chọn Unfriend)
Nhập username: testuser1  (chính mình)
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✗] Cannot unfriend yourself`

### Test 4.2: Unfriend Non-Existent User

**Client 1:**
```
9
nonexistent_user
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✗] User not found`

### Test 4.3: Unfriend User Không Phải Bạn

**Client 1:**
```
9
testuser2  (nếu chưa kết bạn)
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✗] User is not in your friend list`

### Test 4.4: Unfriend Thành Công

**Client 1:**
```
9
testuser2  (sau khi đã kết bạn)
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✓] Unfriended successfully`
- ✅ Verify: `3` (Get Friends) → testuser2 không còn trong list
- ✅ **Bidirection:** Client 2 cũng không còn testuser1 trong list

### Test 4.5: Block User

**Client 1:**
```
14  (Chọn Block User)
Nhập username: testuser2
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✓] User blocked`

**Client 2:**
```
11
testuser1
Test blocked message
```

**Kết quả mong đợi:**
- ✅ Client 2 nhận: `[✗] User has blocked you`

### Test 4.6: Group Permissions

**Client 2 (không phải owner):**
```
15  (Chọn Remove from Group)
Nhập Group ID: GROUP_...
Nhập username: testuser1
```

**Kết quả mong đợi:**
- ✅ Hiển thị: `[✗] Not an admin` hoặc `[✗] Permission denied`

---

## 5. Test Persistence

### Test 5.1: Message Persistence

**Bước 1:** Gửi tin nhắn
- Client 1 gửi: "Persistent message test"

**Bước 2:** Tắt server và client

**Bước 3:** Khởi động lại server

**Bước 4:** Client 1 login và vào chat với testuser2

**Kết quả mong đợi:**
- ✅ Tin nhắn "Persistent message test" vẫn còn trong lịch sử
- ✅ File `../data/messages.txt` có entry với format đúng

### Test 5.2: Account Persistence

**Bước 1:** Tắt server

**Bước 2:** Xóa `../data/account.txt`

**Bước 3:** Khởi động lại server

**Bước 4:** Đăng ký user mới: `testuser3`

**Bước 5:** Tắt server, khởi động lại

**Bước 6:** Login với `testuser3`

**Kết quả mong đợi:**
- ✅ Login thành công
- ✅ File `../data/account.txt` tồn tại và có dữ liệu

### Test 5.3: Pin Message Persistence

**Client 1 (trong group chat):**
```
16  (Chọn Pin Message)
Nhập Group ID: GROUP_...
Nhập message ID hoặc content: Group message test
```

**Bước 2:** Tắt server

**Bước 3:** Khởi động lại server

**Bước 4:** Client 1 vào group chat

**Bước 5:**
```
17  (Chọn Get Pinned Messages)
Nhập Group ID: GROUP_...
```

**Kết quả mong đợi:**
- ✅ Pinned message vẫn còn
- ✅ File `../data/messages.txt` có flag `PINNED=1`

---

## 6. Test UI/UX

### Test 6.1: Chat Bubbles

**Kiểm tra:**
- ✅ Tin nhắn của ME: Màu **xanh lá**, căn **lề phải**
- ✅ Tin nhắn của FRIEND: Màu **trắng/xám**, căn **lề trái**
- ✅ Sử dụng Unicode Box Drawing (`╭─╮`, `│`, `╰─╯`)
- ✅ Header có format: `Sender • Timestamp`

### Test 6.2: UTF-8 Support

**Client 1:**
```
11
testuser2
Chào bạn! 😀 🎉
```

**Kết quả mong đợi:**
- ✅ Tiếng Việt hiển thị đúng: `Chào bạn!`
- ✅ Emoji hiển thị đúng: `😀 🎉`
- ✅ Không bị `?` hoặc ký tự lạ

### Test 6.3: Prompt Cleanliness

**Kiểm tra:**
- ✅ Prompt `[username]:` luôn ở **dòng cuối cùng**
- ✅ Không có prompt rác lơ lửng giữa màn hình
- ✅ Khi tin nhắn đến, prompt được xóa và vẽ lại

### Test 6.4: Sound Notification

**Setup:**
- Client 1 đang ở Menu (không trong chat)
- Client 2 gửi tin cho Client 1

**Kết quả mong đợi:**
- ✅ Có **âm thanh beep** (`\a`)
- ✅ Hiển thị thông báo: `[!] New message from testuser2`

---

## 7. Test Tự Động

### Test 7.1: Chạy Test Suite

**Terminal 1:** Start server
```powershell
cd build
.\server.exe
```

**Terminal 2:** Run tests
```powershell
cd build
.\final_comprehensive_test.exe
```

**Kết quả mong đợi:**
```
[TEST 1] Connection Test
[PASS]   Connection: 2 clients can connect

[TEST 2] Register & Login Test
[PASS]   Register Client A
[PASS]   Register Client B
[PASS]   Login Client A
[PASS]   Login Client B

...

✅ ALL TESTS PASSED! (15/15)
```

### Test 7.2: Advanced Test Suite

```powershell
cd build
.\advanced_test_suite.exe
```

**Kết quả mong đợi:**
- ✅ Most tests pass (14/19 hoặc cao hơn)
- ✅ Edge cases được handle đúng

---

## 8. Checklist Tổng Hợp

### Build & Setup
- [ ] Project compile thành công
- [ ] Server khởi động không lỗi
- [ ] Client kết nối được server
- [ ] Thư mục `data/` và `logs/` được tạo tự động

### Core Features
- [ ] Đăng ký tài khoản
- [ ] Đăng nhập
- [ ] Kết bạn
- [ ] Chat 1-1 real-time
- [ ] Load lịch sử chat (QUAN TRỌNG: hiển thị ngay khi vào phòng)
- [ ] Tạo group
- [ ] Group chat
- [ ] Unfriend (3 edge cases)
- [ ] Block/Unblock user

### Persistence
- [ ] Messages lưu vào file
- [ ] Accounts lưu vào file
- [ ] Pin messages persist sau restart
- [ ] Activity log ghi đúng

### UI/UX
- [ ] Chat bubbles đẹp (màu sắc, căn lề)
- [ ] UTF-8 support (Tiếng Việt, Emoji)
- [ ] Prompt không bị rác
- [ ] Sound notification hoạt động
- [ ] Unicode box drawing hiển thị đúng

### Edge Cases
- [ ] Self-unfriend bị reject
- [ ] Unfriend non-existent user bị reject
- [ ] Unfriend non-friend bị reject
- [ ] Blocked user không gửi được tin
- [ ] Non-owner không kick được member

### Automated Tests
- [ ] Basic test suite: 15/15 PASS
- [ ] Advanced test suite: 14+/19 PASS

---

## 9. Troubleshooting

### Lỗi: "Cannot connect to server"
- **Nguyên nhân:** Server chưa chạy
- **Giải pháp:** Start server trước: `cd build && .\server.exe`

### Lỗi: "Port already in use"
- **Nguyên nhân:** Server đang chạy ở terminal khác
- **Giải pháp:** Tìm và kill process:
```powershell
Get-Process | Where-Object {$_.ProcessName -like "*server*"} | Stop-Process -Force
```

### Lỗi: "File not found" khi chạy từ build/
- **Nguyên nhân:** Paths chưa được fix
- **Giải pháp:** Đảm bảo code dùng `../data/` và `../logs/`

### Lỗi: Emoji không hiển thị
- **Nguyên nhân:** Terminal không hỗ trợ UTF-8
- **Giải pháp:** Dùng Windows Terminal hoặc PowerShell mới

### Lỗi: Lịch sử không hiển thị
- **Nguyên nhân:** Server chưa implement đúng logic
- **Giải pháp:** Kiểm tra `CMD_GET_CHAT_HISTORY` trong server.c

---

## 10. Kết Luận

Sau khi hoàn thành tất cả tests trên, dự án đã sẵn sàng để:
- ✅ Demo cho giảng viên
- ✅ Nộp bài
- ✅ Chấm điểm (14/14 điểm expected)

**Lưu ý:** Nếu bất kỳ test nào FAIL, hãy ghi lại và báo cáo để fix.

