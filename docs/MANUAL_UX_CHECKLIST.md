# Manual UX Checklist - Visual & Audio Verification

## Task 3: Manual UX Checklist

**Lưu ý:** Checklist này dành cho tester thực hiện bằng mắt và tai, không thể tự động hóa.

---

### ✅ Checklist 1: Giao diện Bong bóng Chat (Chat Bubbles)

**Môi trường test:** 
- Chạy 2 client, đăng nhập 2 user khác nhau
- Vào phòng chat 1-1

**Kiểm tra:**

- [ ] **Màu sắc:**
  - [ ] Tin nhắn của **ME** (bạn): Màu **Xanh lá** (Green) - `COLOR_ME`
  - [ ] Tin nhắn của **FRIEND** (bạn bè): Màu **Trắng/Xám** - `COLOR_FRIEND`
  - [ ] Không có màu lạ hoặc bị lỗi encoding

- [ ] **Căn lề (Alignment):**
  - [ ] Tin nhắn của **ME**: Căn **lề phải** (Right Align)
  - [ ] Tin nhắn của **FRIEND**: Căn **lề trái** (Left Align)
  - [ ] Không có tin nhắn nào bị lệch giữa màn hình

- [ ] **Khung bong bóng:**
  - [ ] Sử dụng **Unicode Box Drawing** (`╭─╮`, `╰─╯`, `│`)
  - [ ] Không còn ký tự ASCII cũ (`+---`, `|`)
  - [ ] Khung bo tròn đẹp mắt

- [ ] **Header trong bong bóng:**
  - [ ] Mỗi bong bóng có header: `Sender • Timestamp` (ví dụ: `test1 • 18:49:25`)
  - [ ] Header có màu xám (`COLOR_SYSTEM`)
  - [ ] Có đường phân cách giữa header và content

- [ ] **Word Wrapping:**
  - [ ] Tin nhắn dài tự động xuống dòng
  - [ ] Không bị cắt giữa chữ
  - [ ] Bong bóng có chiều rộng hợp lý (không quá rộng)

---

### ✅ Checklist 2: Hiển thị Emoji trên Windows Terminal

**Môi trường test:**
- Windows Terminal hoặc PowerShell
- Client đã chạy

**Kiểm tra:**

- [ ] **Gửi Emoji:**
  - [ ] Gõ tin nhắn chứa emoji: `Hello 😀 🎉 Test`
  - [ ] Emoji hiển thị **đúng** (không bị `?` hoặc ký tự lạ)
  - [ ] Không bị lỗi encoding

- [ ] **Gửi Tiếng Việt có dấu:**
  - [ ] Gõ: `Chào bạn, tôi đang test`
  - [ ] Tiếng Việt hiển thị **đúng** (không bị `Chao ban`)
  - [ ] Không bị lỗi encoding

- [ ] **Box Drawing Characters:**
  - [ ] Các ký tự `╔ ═ ╗ ║ ╚ ╝` hiển thị đúng
  - [ ] Không bị thay thế bằng ký tự ASCII

**Lưu ý:** Nếu emoji không hiển thị, kiểm tra:
- Terminal có hỗ trợ UTF-8 không?
- `SetConsoleOutputCP(65001)` đã được gọi chưa?

---

### ✅ Checklist 3: Thông báo Âm thanh (Sound Notification)

**Môi trường test:**
- 2 client đang chạy
- Client A đang ở **Menu** (không trong phòng chat)
- Client B gửi tin cho A

**Kiểm tra:**

- [ ] **Sound khi nhận tin:**
  - [ ] Khi B gửi tin cho A (A đang ở menu)
  - [ ] Client A phát ra **âm thanh beep** (`\a`)
  - [ ] Âm thanh rõ ràng, không bị cắt

- [ ] **Visual Notification:**
  - [ ] Sau khi beep, hiển thị thông báo: `[!] New message from B. Go to chat to view.`
  - [ ] Thông báo có màu vàng (`COLOR_WARNING`)
  - [ ] Không bị che khuất bởi menu

- [ ] **Trong phòng chat:**
  - [ ] Khi A đang chat với B, B gửi tin mới
  - [ ] Có beep sound
  - [ ] Tin nhắn hiển thị ngay trong bong bóng (không chỉ notification)

---

### ✅ Checklist 4: Prompt Cleanliness (Không có "Dirty Prompt")

**Môi trường test:**
- 2 client đang chat với nhau

**Kiểm tra:**

- [ ] **Không có prompt rác:**
  - [ ] Không có dòng `[username]:` lơ lửng giữa màn hình
  - [ ] Không có dòng `-[username]: _________` kỳ quặc
  - [ ] Prompt `[username]:` luôn ở **dòng cuối cùng**

- [ ] **Khi tin nhắn đến:**
  - [ ] Dòng prompt cũ được xóa trước khi in bong bóng mới
  - [ ] Sau khi in bong bóng, prompt được vẽ lại ở dòng cuối
  - [ ] Không có xung đột giữa prompt và tin nhắn

- [ ] **Khi gửi tin nhắn:**
  - [ ] Sau khi gõ Enter, prompt được xóa
  - [ ] Bong bóng tin nhắn của bạn hiển thị ngay
  - [ ] Prompt mới xuất hiện ở dòng cuối

---

### ✅ Checklist 5: History Loading (Load Lịch sử)

**Môi trường test:**
- Client A gửi vài tin cho B (B đang ở menu)
- B vào phòng chat với A

**Kiểm tra:**

- [ ] **Lịch sử hiển thị:**
  - [ ] Khi B vào phòng chat, **ngay lập tức** thấy lịch sử cũ
  - [ ] Không phải màn hình trống trơn
  - [ ] Tất cả tin nhắn 2 chiều (A→B và B→A) đều hiển thị

- [ ] **Thứ tự tin nhắn:**
  - [ ] Tin nhắn cũ ở trên, tin nhắn mới ở dưới
  - [ ] Timestamp hiển thị đúng
  - [ ] Không có tin nhắn bị trùng lặp

---

### ✅ Checklist 6: Disconnect Notification

**Môi trường test:**
- 2 client đang chat với nhau
- Client A disconnect (gõ menu 21 hoặc tắt app)

**Kiểm tra:**

- [ ] **Client B nhận thông báo:**
  - [ ] Client B thấy thông báo: `User has disconnected` hoặc tương tự
  - [ ] Thông báo có màu system (xám)
  - [ ] Thông báo không làm vỡ layout

---

## Quick Test Script (Manual)

```powershell
# Terminal 1: Server
cd C:\Users\Admin\Desktop\Lap_Trinh_Mang\Chat_App
.\server.exe

# Terminal 2: Client A
cd C:\Users\Admin\Desktop\Lap_Trinh_Mang\Chat_App
.\client.exe
# Login: testA / passwordA
# Menu 11 → testB (Enter chat room)
# Gửi tin: "Hello 😀 🎉"
# Gửi tin: "Chào bạn"

# Terminal 3: Client B
cd C:\Users\Admin\Desktop\Lap_Trinh_Mang\Chat_App
.\client.exe
# Login: testB / passwordB
# Menu 11 → testA (Enter chat room)
# Quan sát: Lịch sử có hiển thị không?
# Gửi tin: "Hi A"
# Quan sát: Bong bóng màu gì? Căn trái hay phải?
# Quan sát: Prompt có ở dòng cuối không?
```

---

## Expected Results

- ✅ Bong bóng: Xanh (ME, căn phải) / Trắng (FRIEND, căn trái)
- ✅ Emoji: Hiển thị đúng `😀 🎉`
- ✅ Tiếng Việt: Hiển thị đúng `Chào bạn`
- ✅ Sound: Có beep khi nhận tin
- ✅ Prompt: Luôn ở dòng cuối, không có rác
- ✅ History: Hiển thị ngay khi vào phòng chat

---

## Notes

- Nếu emoji không hiển thị: Kiểm tra Windows Terminal có hỗ trợ UTF-8 không
- Nếu sound không phát: Kiểm tra volume system
- Nếu prompt bị rác: Có thể do terminal không hỗ trợ ANSI escape codes đầy đủ

