# Hướng Dẫn Test Nhanh - Chat Application

## 🚀 Cách Test Nhanh Nhất

### Option 1: Tự Động (Khuyến Nghị)

```powershell
# Chạy script tự động
.\start_test.ps1
.\auto_test.ps1
```

Script sẽ:
1. ✅ Tạo thư mục cần thiết
2. ✅ Compile project
3. ✅ Start server và client tự động

### Option 2: Thủ Công

**Terminal 1 - Server:**
```powershell
cd build
.\server.exe
```

**Terminal 2 - Client:**
```powershell
cd build
.\client.exe
```

---

## 📋 Test Checklist Nhanh

### Test 1: Register & Login (30 giây)
```
1 → testuser1 → password123
2 → testuser1 → password123
```
**✅ Kết quả:** Login successful

### Test 2: Chat 1-1 (1 phút)
**Mở Terminal 3:**
```
1 → testuser2 → password456
2 → testuser2 → password456
4 → testuser1  (Add friend)
11 → testuser1 (Chat)
   → Hello! 😀
```

**Terminal 2 (testuser1):**
- ✅ Nhận tin nhắn ngay
- ✅ Bong bóng chat đẹp
- ✅ Emoji hiển thị đúng

### Test 3: Load History (QUAN TRỌNG)
**Terminal 2:**
```
11 → testuser2
```

**✅ Kết quả:** Lịch sử "Hello! 😀" hiển thị NGAY (không phải màn hình trống)

---

## ⚠️ Test Quan Trọng Nhất

**Load History Test:**
1. User A gửi tin cho User B (B đang ở menu)
2. User B vào phòng chat với A
3. **Kỳ vọng:** Lịch sử hiển thị NGAY

**Nếu FAIL:** Bug nghiêm trọng!

---

## 🛠️ Troubleshooting

### Lỗi: "Cannot connect"
→ Server chưa chạy, chạy `.\start_test.ps1` trước

### Lỗi: "File not found"
→ Đảm bảo chạy từ `build/` directory

### Lỗi: Compile failed
→ Chạy `.\start_test.ps1` để compile lại

---

## 📖 Chi Tiết Hơn

Xem `TESTING_GUIDE.md` để có hướng dẫn test toàn diện.

