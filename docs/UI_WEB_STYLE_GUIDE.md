# UI WEB-STYLE UPGRADE - HƯỚNG DẪN

## Tổng quan
UI đã được nâng cấp lên mức "web-style" với bubble messages và menu dạng card/dashboard.

---

## Tính năng mới

### 1. Bubble Messages (Chat Bubbles)
Messages hiển thị dạng bubble giống ứng dụng nhắn tin hiện đại:

- **My Messages (Me)**:
  - Căn lề phải
  - Màu Cyan (Bold)
  - Border rounded: `╭ ─ ╮ │ ╯`
  - Tên hiển thị bên dưới góc phải

- **Friend Messages**:
  - Căn lề trái
  - Màu Yellow (Bold)
  - Border rounded: `╭ ─ ╮ │ ╯`
  - Tên hiển thị bên dưới góc trái

- **System Messages**:
  - Màu Grey
  - Format đơn giản: `[HH:MM:SS] Message`

### 2. Fancy Menu (Card/Dashboard Style)
Menu hiển thị dạng card với các section rõ ràng:
- Account & Social (1-10)
- Chatting (11-20)
- System (21-22)

### 3. Navbar
Header có thể hiển thị username (tùy chọn)

---

## Cách sử dụng

### Hàm mới trong `ui.h`:

```c
// Vẽ bubble message
void ui_print_message_bubble(const char* sender, const char* content, int is_me, const char* timestamp);

// Vẽ bubble core (internal)
void ui_draw_bubble(const char* content, int align_right, const char* color, int max_width);

// Menu fancy (card style)
void ui_print_menu_fancy(void);

// Navbar với username
void ui_print_navbar(const char* title, const char* username);
```

### Ví dụ sử dụng:

```c
// In message bubble
ui_print_message_bubble("userA", "Hello! How are you?", 0, "14:30:25");
// → Friend message (căn trái, màu Yellow)

ui_print_message_bubble("me", "I'm fine, thanks!", 1, "14:30:30");
// → My message (căn phải, màu Cyan)

// In navbar
ui_print_navbar("Chat App", "username");
```

---

## Tự động tích hợp

### Client đã tự động sử dụng bubble messages:
- Khi nhận `CMD_RECEIVE_MESSAGE`:
  - Nếu `sender == current_username` → `is_me = 1` (căn phải, Cyan)
  - Nếu `sender != current_username` → `is_me = 0` (căn trái, Yellow)
  - Nếu system message → format đơn giản (Grey)

---

## Màu sắc

### Color Palette:
- **COLOR_ME**: Bold Cyan (`\033[1;36m`) - My messages
- **COLOR_FRIEND**: Bold Yellow (`\033[1;33m`) - Friend messages
- **COLOR_SYSTEM**: Grey (`\033[90m`) - System messages
- **COLOR_SUCCESS**: Green - Success notifications
- **COLOR_ERROR**: Red - Error notifications
- **COLOR_INFO**: Cyan - Info messages
- **COLOR_HEADER**: Bold Cyan - Headers

---

## Box Drawing Characters

### Rounded (Bubbles):
- `╭` Top Left
- `─` Horizontal
- `╮` Top Right
- `│` Vertical
- `╯` Bottom Right
- `╰` Bottom Left

### Double (Headers/Menus):
- `╔` Top Left
- `═` Horizontal
- `╗` Top Right
- `║` Vertical
- `╝` Bottom Right
- `╚` Bottom Left

---

## Word Wrapping

Bubble messages tự động wrap text:
- Max width: 50 characters per line
- Tự động break tại space (word boundary)
- Nếu không có space, break tại max width
- Hỗ trợ tối đa 10 dòng

---

## Test

### Để test bubble messages:
1. Chạy server: `.\server.exe`
2. Chạy 2 clients:
   - Client 1: Login `user1`
   - Client 2: Login `user2`
3. Add friend giữa 2 users
4. Gửi message:
   - Client 1 gửi → Hiển thị bubble căn phải (Cyan)
   - Client 2 nhận → Hiển thị bubble căn trái (Yellow)
   - Client 2 gửi → Hiển thị bubble căn phải (Cyan)
   - Client 1 nhận → Hiển thị bubble căn trái (Yellow)

### Kết quả mong đợi:
```
[14:30:25] 
╭─────────────────────────────╮
│ Hello! How are you?         │
╰─────────────────────────────╯
userA

[14:30:30] 
                                    ╭──────────────────────╮
                                    │ I'm fine, thanks!    │
                                    ╰──────────────────────╯
                                                          me
```

---

## Lưu ý

1. **UTF-8 Support**: Đảm bảo `SetConsoleOutputCP(65001)` được gọi (tự động trong `ui_init()`)

2. **Terminal Width**: Mặc định 80 characters, có thể điều chỉnh trong `ui.c`:
   ```c
   #define TERMINAL_WIDTH 80
   ```

3. **Bubble Width**: Mặc định 50 characters, có thể điều chỉnh:
   ```c
   #define BUBBLE_MAX_WIDTH 50
   ```

4. **Right Indent**: Khoảng cách từ phải cho "me" messages:
   ```c
   #define BUBBLE_INDENT_RIGHT 30
   ```

---

## Troubleshooting

### Box characters hiển thị sai:
- Kiểm tra terminal có support UTF-8 không
- Thử dùng Windows Terminal hoặc PowerShell mới
- Đảm bảo `ui_init()` được gọi

### Màu sắc không hiển thị:
- Kiểm tra terminal có support ANSI escape codes không
- Windows 10+ nên support mặc định
- Linux/Unix support native

### Bubble alignment sai:
- Điều chỉnh `TERMINAL_WIDTH` và `BUBBLE_INDENT_RIGHT` trong `ui.c`
- Kiểm tra terminal width thực tế

---

**Ngày tạo**: 2025-11-30  
**Version**: UI Web-Style v1.0


