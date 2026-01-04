# 🌐 LAN CHAT DEMO GUIDE - WSL2 & LINUX

Hướng dẫn chi tiết cách thiết lập và kết nối ứng dụng Chat trong mạng nội bộ (LAN) sử dụng môi trường **Linux (WSL2)**.

---

## 🛠 I. CHUẨN BỊ (Prerequisites)

| Đối tượng | Yêu cầu |
| :--- | :--- |
| **Mạng** | Hai máy tính kết nối cùng một điểm truy cập Wi-Fi hoặc LAN. |
| **Môi trường** | Đã cài đặt Linux/WSL2, `gcc`, `make`. |
| **Biên dịch** | Cả 2 máy đã chạy `make` thành công để có file thực thi `./server` và `./client`. |

---

## 🖥 II. THIẾT LẬP TRÊN MÁY CHỦ (SERVER MACHINE)

### 1️⃣ Lấy thông số địa chỉ IP
Trên **Máy 1** (vừa chạy Server vừa chạy Windows để bắc cầu):

*   **IP Linux (WSL2):** Mở Terminal Linux và gõ:
    ```bash
    hostname -I
    ```
    *(Ví dụ: `172.30.177.162`) -> Gọi là **WSL_IP***
*   **IP Windows (Mạng Wifi):** Mở PowerShell Windows và gõ:
    ```powershell
    ipconfig
    ```
    *(Tìm dòng IPv4 Address của Wi-Fi, ví dụ: `192.168.1.175`) -> Gọi là **LAN_IP***

### 2️⃣ Cấu hình Port Forwarding (Bắc cầu)
*Mở **PowerShell (Admin)** và thực hiện lệnh bên dưới để Windows chuyển tiếp dữ liệu vào WSL2:*

```powershell
# Thay <WSL_IP> bằng IP thực tế bạn vừa lấy
netsh interface portproxy add v4tov4 listenport=8080 listenaddress=0.0.0.0 connectport=8080 connectaddress=<WSL_IP>
```

### 3️⃣ Mở Tường lửa (Firewall)
*Vẫn tại **PowerShell (Admin)**, cấp quyền cho cổng 8080 được nhận kết nối từ ngoài:*

```powershell
netsh advfirewall firewall add rule name="ChatApp_8080" dir=in action=allow protocol=TCP localport=8080
```

---

## 🚀 III. QUY TRÌNH CHẠY DEMO

### 🟦 1. Trên Máy 1 (Server & Client A)
*   **Terminal 1 (Server):** 
    ```bash
    ./server
    ```
*   **Terminal 2 (Client A):** 
    ```bash
    ./client
    # Gõ IP: 127.0.0.1 (hoặc chỉ cần Enter)
    ```

### 🟩 2. Trên Máy 2 (Client B)
*   Mở Terminal Linux/Windows và chạy:
    ```bash
    ./client
    ```
*   **Khi hỏi Server IP:** Nhập chính xác địa chỉ **LAN_IP** của Máy 1 (Ví dụ: `192.168.1.175`).

---

## 📝 IV. SCANNER CÁC TÍNH NĂNG DEMO
1.  ✨ **Đăng ký:** Tài khoản `A` (Máy 1) và `B` (Máy 2).
2.  🤝 **Kết bạn:** `A` gửi lời mời -> `B` chấp nhận (Option 5).
3.  💬 **Nhắn tin:** Cả hai vào Option 7 và bắt đầu chat thời gian thực qua mạng LAN.
4.  👥 **Nhóm:** Tạo nhóm và mời thành viên để thấy sức mạnh của Multi-client.

---

## 🆘 V. XỬ LÝ SỰ CỐ (Troubleshooting)

> [!IMPORTANT]
> **Không kết nối được?** 
> 1. Kiểm tra lệnh `netsh` xem đã nhập đúng IP của WSL chưa.
> 2. Đảm bảo 2 máy ping được nhau (`ping <LAN_IP>`).
> 3. Kiểm tra xem file `./server` có đang báo lỗi "Address already in use" không.

> [!TIP]
> **Dọn dẹp môi trường sau khi test:**
> Để xóa cấu hình cầu nối mạng, dùng lệnh (PowerShell Admin):
> `netsh interface portproxy delete v4tov4 listenport=8080 listenaddress=0.0.0.0`
*Xóa cấu hình Proxy:** Nếu muốn xóa cầu nối sau khi test:
  ```powershell
  netsh interface portproxy delete v4tov4 listenport=8080 listenaddress=0.0.0.0
  ```
