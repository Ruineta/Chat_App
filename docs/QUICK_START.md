# QUICK START - HƯỚNG DẪN CHẠY NHANH

## ⚠️ QUAN TRỌNG: Phải chạy Server TRƯỚC Client!

---

## BƯỚC 1: Khởi động Server

Mở **Terminal 1** (PowerShell hoặc CMD):

```powershell
cd C:\Users\Admin\Desktop\Lap_Trinh_Mang\Chat_App
.\server.exe
```

**Kết quả mong đợi:**
```
Server listening on port 8080
```

**Nếu thấy lỗi:**
- `Port already in use`: Có server khác đang chạy → Dừng process cũ:
  ```powershell
  Get-Process | Where-Object {$_.ProcessName -eq "server"} | Stop-Process -Force
  ```

---

## BƯỚC 2: Khởi động Client

Mở **Terminal 2** (PowerShell hoặc CMD MỚI - không đóng Terminal 1):

```powershell
cd C:\Users\Admin\Desktop\Lap_Trinh_Mang\Chat_App
.\client.exe
```

**Kết quả mong đợi:**
```
Connected to server

╔══════════════════════════════════════════════════════════╗
║              Chat Application Menu                       ║
╚══════════════════════════════════════════════════════════╝

╠ Account & Social ════════════════════════════════════════╣
════════════════════════════════════════════════════════════
  1. Register
  2. Login
  ...
```

**Nếu thấy lỗi:**
- `Connection failed: 10061`: Server chưa chạy → Quay lại Bước 1
- `Connection failed: 10048`: Port đã được sử dụng → Dừng server cũ và chạy lại

---

## BƯỚC 3: Test UI Framework

Sau khi menu hiển thị, kiểm tra:

### ✅ Menu UI
- [ ] Menu có box style: `╔ ═ ╗ ║ ╚ ╝`
- [ ] Menu chia 3 nhóm rõ ràng
- [ ] Section headers có màu Cyan

### ✅ Input Prompts
- [ ] Chọn menu `1` (Register)
- [ ] Kiểm tra: `> Username: ` (có màu)
- [ ] Kiểm tra: `> Password: ` (có màu)

### ✅ Notifications
- [ ] Register thành công → `[✓] Success` (màu Green)
- [ ] Login sai → `[✗] Error` (màu Red)

---

## SCRIPT TỰ ĐỘNG (Khuyến nghị)

Thay vì chạy manual, bạn có thể dùng script:

```powershell
.\test_ui.ps1
```

Script sẽ:
1. Compile lại toàn bộ
2. Khởi động server tự động
3. Mở client để test

---

## TROUBLESHOOTING

### Lỗi: Connection failed: 10061
**Nguyên nhân:** Server chưa chạy hoặc chưa sẵn sàng

**Giải pháp:**
1. Kiểm tra server có đang chạy không:
   ```powershell
   Get-Process | Where-Object {$_.ProcessName -eq "server"}
   ```
2. Nếu không có, chạy server:
   ```powershell
   .\server.exe
   ```
3. Đợi 1-2 giây để server khởi động xong
4. Chạy lại client

### Lỗi: Port already in use
**Nguyên nhân:** Có server khác đang chạy trên port 8080

**Giải pháp:**
```powershell
Get-Process | Where-Object {$_.ProcessName -eq "server"} | Stop-Process -Force
.\server.exe
```

### Lỗi: Warnings về %lld
**Nguyên nhân:** Windows MinGW không support `%lld`

**Giải pháp:** Đã được sửa trong code, compile lại:
```powershell
gcc -Wall -Wextra -std=c11 -c server.c -o server.o
gcc -Wall -Wextra -std=c11 -o server.exe server.o common.o -lws2_32
```

### Menu không hiển thị màu/box characters
**Nguyên nhân:** Terminal không support UTF-8 hoặc ANSI escape codes

**Giải pháp:**
1. Dùng Windows Terminal (khuyến nghị) hoặc PowerShell mới
2. Kiểm tra Windows version (Windows 10+ nên support)
3. Nếu vẫn không được, màu sắc vẫn hoạt động, chỉ box characters có thể không hiển thị

---

## CHECKLIST NHANH

- [ ] Server đang chạy (Terminal 1)
- [ ] Client kết nối thành công (Terminal 2)
- [ ] Menu hiển thị đẹp với box style
- [ ] Input prompts có màu
- [ ] Notifications có màu và icon

---

**Lưu ý:** Luôn chạy Server TRƯỚC Client!


