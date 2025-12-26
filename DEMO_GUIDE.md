# Hướng dẫn Demo trên 2 Máy tính (Linux/WSL)

Làm theo các bước sau để chạy ứng dụng trên 2 máy tính kết nối cùng mạng LAN/Wifi.

## 1. Chuẩn bị
*   Bạn cần có 2 máy tính (gọi là Máy A và Máy B).
*   Đảm bảo cả 2 máy đã có file thực thi `server` và `client` (do tôi vừa tạo lại).
*   Đảm bảo cả 2 máy đều chạy hệ điều hành Linux (hoặc WSL).
*   Đảm bảo 2 máy "ping" thấy nhau.

## 2. Tại Máy A (Chạy Server)
1.  Mở Terminal (Ctrl+Alt+T).
2.  Tìm địa chỉ IP của máy này:
    *   Chạy lệnh: `ip addr` hoặc `ifconfig`.
    *   Tìm dòng có `inet` (ví dụ: `192.168.1.100` hoặc `10.0.0.5`).
3.  Chạy Server:
    ```bash
    ./server
    ```
    *Server sẽ báo: Server POLL-based started on port 8080*
4.  Ghi lại địa chỉ IP này để đưa cho Máy B.

## 3. Tại Máy B (Chạy Client)
1.  Copy file thực thi `client` sang máy này (hoặc copy source code `client.c`, `common.c`, `common.h` và biên dịch lại bằng `gcc client.c common.c -o client`).
2.  Chạy Client:
    ```bash
    ./client
    ```
3.  Khi màn hình hiện: `Server IP [127.0.0.1]:`
    *   Nhập địa chỉ IP của Máy A (ví dụ: `192.168.1.100`).
    *   Nhấn **Enter**.
4.  Đăng nhập và chat bình thường!

## 4. Tại Máy A (Cũng có thể chạy Client)
*   Bạn có thể mở thêm một Terminal khác tại Máy A để chạy `./client` và nhập IP `127.0.0.1` để tự chat với chính mình hoặc kiểm tra kết nối.

## Lưu ý về Tường lửa (Firewall)
*   Nếu Máy B không kết nối được, hãy kiểm tra Firewall trên Máy A xem có chặn cổng **8080** không.
*   Lệnh mở cổng tạm thời (trên Ubuntu/Debian): `sudo ufw allow 8080`.
