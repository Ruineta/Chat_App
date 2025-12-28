# Kịch Bản Demo Đồ Án Chat Application

Tài liệu này giúp bạn dẫn dắt buổi demo một cách chuyên nghiệp, làm nổi bật các điểm mạnh kỹ thuật của dự án.

## 1. Giới thiệu Kiến trúc (Phần quan trọng nhất)
*   **Điểm nhấn**: "Dự án của em không dùng Đa luồng (Multi-threading) theo cách truyền thống mà sử dụng **I/O Multiplexing với `poll()`**."
*   **Giải thích**: "Điều này giúp Server xử lý hàng nghìn kết nối chỉ với 1 luồng duy nhất, cực kỳ tiết kiệm tài nguyên và không bao giờ bị lỗi xung đột dữ liệu (Race Condition)."

## 2. Các bước thực hiện Demo

### Bước 1: Khởi động hệ thống
1.  Bật **Server**: `./server`
    *   *Nói: "Server đang lắng nghe ở cổng 8080 bằng vòng lặp sự kiện (Event Loop)."*
2.  Bật **Client A** (Máy 1) và **Client B** (Máy 2): `./client`
    *   *Nhập IP Server để kết nối.*

### Bước 2: Đăng nhập & Offline Messages
1.  Đăng nhập **giang1b** ở Client A.
2.  Gửi tin nhắn cho **giang2b** (lúc này đang Offline).
    *   *Nói: "Hệ thống hỗ trợ gửi tin nhắn Offline, tin nhắn sẽ được lưu vào cơ sở dữ liệu file."*
3.  Đăng nhập **giang2b** ở Client B.
    *   *Quan sát: Màn hình chào mừng hiện thông báo: "Welcome back! You have unread messages from giang1b..."*
    *   *Nói: "Khi đăng nhập, Server tự động quét lịch sử và thông báo breakdown chi tiết số tin nhắn chưa đọc."*

### Bước 3: Chat liền mạch (Seamless Chat)
1.  Ở Client A, chọn **Option 7** (Seamless Chat) -> Nhập `giang2b`.
2.  Ở Client B, chọn **Option 7** -> Nhập `giang1b`.
3.  Thực hiện chat qua lại.
    *   *Nói: "Nhờ cơ chế `poll()` ở Client, người dùng có thể nhận tin nhắn mới ngay lập tức mà không ảnh hưởng đến việc đang gõ phím. Giao diện được thiết kế chuyên nghiệp với header và footer cố định."*

### Bước 4: Lịch sử Chat (History)
1.  Thoát chat (`/exit`) rồi vào lại.
2.  Quan sát các tin nhắn cũ hiện lên với nhãn `[History YYYY-MM-DD...]`.
    *   *Nói: "Hệ thống tự động tải lịch sử 3 ngày gần nhất. Server sử dụng bộ giải mã đặc biệt (Dual-Parser) để đọc được cả các định dạng tin nhắn cũ và mới, đảm bảo tính tương thích ngược."*

### Bước 5: Quản lý bạn bè & Chặn
1.  Thử tính năng **Option 3** (Check Status) để xem bạn bè đang Online hay Offline.
2.  Thử tính năng **Option 8** (Block User). Sau khi chặn, thử gửi tin nhắn sẽ báo lỗi "Cannot send message".
    *   *Nói: "Toàn bộ trạng thái chặn và bạn bè được cập nhật Real-time trong bộ nhớ Server."*

## 3. Kết thúc
*   Nhấn **Option 0** để thoát.
*   *Nói: "Dự án đã đáp ứng đầy đủ yêu cầu và áp dụng các kỹ thuật lập trình hệ thống tối ưu nhất trên Linux."*
