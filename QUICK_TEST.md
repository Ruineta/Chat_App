# Quick Test Guide - Test Nhanh Dự Án

## ⚡ Test Nhanh 5 Phút

### Bước 1: Build Project (1 phút)

```powershell
# Compile
gcc -Wall -Wextra -std=c11 -Iinclude -c src/common.c -o build/common.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/server.c -o build/server.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/client.c -o build/client.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/ui.c -o build/ui.o
gcc -Wall -Wextra -std=c11 -Iinclude -o build/server.exe build/server.o build/common.o -lws2_32
gcc -Wall -Wextra -std=c11 -Iinclude -o build/client.exe build/client.o build/common.o build/ui.o -lws2_32
```

**✅ Kết quả:** `build/server.exe` và `build/client.exe` được tạo

---

### Bước 2: Start Server (30 giây)

**Terminal 1:**
```powershell
cd build
.\server.exe
```

**✅ Kết quả:** Server chạy, hiển thị "Waiting for clients..."

---

### Bước 3: Test Cơ Bản (3 phút)

**Terminal 2:**
```powershell
cd build
.\client.exe
```

#### Test 3.1: Register & Login
```
1  → testuser1 → password123
2  → testuser1 → password123
```

**✅ Kết quả:** Login successful

#### Test 3.2: Chat 1-1 (Mở Terminal 3)
**Terminal 3:**
```powershell
cd build
.\client.exe
```
```
1  → testuser2 → password456
2  → testuser2 → password456
4  → testuser1  (Add friend)
11 → testuser1  (Chat)
   → Hello! 😀
```

**Terminal 2 (testuser1):**
- ✅ Nhận được tin nhắn ngay
- ✅ Bong bóng chat đẹp
- ✅ Emoji hiển thị đúng

#### Test 3.3: Load History
**Terminal 2:**
```
11 → testuser2
```

**✅ Kết quả:** Lịch sử "Hello! 😀" hiển thị NGAY (không phải màn hình trống)

---

## 🎯 Test Quan Trọng Nhất

### ✅ Test Load History (CRITICAL)

**Kịch bản:**
1. User A gửi tin cho User B (B đang ở menu)
2. User B vào phòng chat với A
3. **Kỳ vọng:** Lịch sử hiển thị NGAY, không phải màn hình trống

**Nếu FAIL:** Đây là bug nghiêm trọng cần fix ngay!

---

## 📋 Checklist Nhanh

- [ ] Server compile và chạy
- [ ] Client compile và kết nối được
- [ ] Register/Login hoạt động
- [ ] Chat 1-1 real-time
- [ ] **Load history ngay khi vào phòng** ⚠️ QUAN TRỌNG
- [ ] Emoji hiển thị đúng
- [ ] Tiếng Việt hiển thị đúng
- [ ] Bong bóng chat đẹp (màu, căn lề)

---

## 🚨 Nếu Có Lỗi

### Lỗi: "Cannot connect"
→ Server chưa chạy, start server trước

### Lỗi: "File not found"
→ Đảm bảo chạy từ `build/` directory

### Lỗi: History không hiển thị
→ Bug nghiêm trọng, cần fix logic `CMD_GET_CHAT_HISTORY`

---

## 📖 Test Chi Tiết

Xem file `TESTING_GUIDE.md` để có hướng dẫn test toàn diện hơn.

