# Network Configuration Guide

Hướng dẫn cấu hình mạng để chạy Chat Application trên nhiều máy tính trong cùng mạng LAN.

---

## 1. Lấy IP Address của Server

### Windows:
```powershell
ipconfig
```
Tìm dòng **IPv4 Address** (ví dụ: `192.168.1.100`)

### Linux:
```bash
ifconfig
# hoặc
ip addr
```
Tìm dòng **inet** trong interface (ví dụ: `192.168.1.100`)

---

## 2. Kiểm tra Kết nối

### Ping Test:
Từ máy client, ping đến server:
```bash
# Windows
ping 192.168.1.100

# Linux
ping 192.168.1.100
```

Nếu ping thành công, kết nối mạng OK.

---

## 3. Cấu hình Firewall

### Windows Firewall:
1. Mở **Windows Defender Firewall with Advanced Security**
2. Chọn **Inbound Rules** → **New Rule**
3. Chọn **Port** → **Next**
4. Chọn **TCP**, nhập port **8080** → **Next**
5. Chọn **Allow the connection** → **Next**
6. Chọn tất cả profiles (Domain, Private, Public) → **Next**
7. Đặt tên: "Chat App Server Port 8080" → **Finish**

Hoặc dùng PowerShell (chạy với quyền Administrator):
```powershell
New-NetFirewallRule -DisplayName "Chat App Server" -Direction Inbound -LocalPort 8080 -Protocol TCP -Action Allow
```

### Linux Firewall (UFW):
```bash
sudo ufw allow 8080/tcp
sudo ufw reload
```

### Linux Firewall (iptables):
```bash
sudo iptables -A INPUT -p tcp --dport 8080 -j ACCEPT
sudo iptables-save
```

---

## 4. Cấu hình Virtual Machine Network

### Option 1: Bridged Network (Khuyến nghị)
- VM có IP riêng trong LAN
- Có thể kết nối trực tiếp từ máy host hoặc máy khác trong LAN
- Cấu hình: VM Settings → Network → Adapter 1 → Bridged

### Option 2: NAT Network
- VM dùng IP của host
- Cần port forwarding phức tạp hơn
- Không khuyến nghị cho mạng LAN

---

## 5. Chạy Server

Server đã được cấu hình để bind `INADDR_ANY` (0.0.0.0), nghĩa là:
- Lắng nghe trên tất cả network interfaces
- Có thể nhận kết nối từ bất kỳ IP nào trong mạng

Chạy server:
```bash
# Windows
.\server.exe

# Linux
./server
```

Server sẽ hiển thị:
```
Server started on port 8080
Waiting for clients...
```

---

## 6. Kết nối từ Client

### Từ máy khác trong cùng mạng LAN:
```bash
# Windows
.\client.exe

# Khi được hỏi server IP, nhập IP của server (ví dụ: 192.168.1.100)

# Linux
./client

# Khi được hỏi server IP, nhập IP của server
```

### Từ cùng máy (localhost):
```bash
# Nhập: 127.0.0.1 hoặc localhost
```

---

## 7. Troubleshooting

### Vấn đề: Client không kết nối được
**Giải pháp:**
1. Kiểm tra server đang chạy: `netstat -an | findstr 8080` (Windows) hoặc `netstat -tuln | grep 8080` (Linux)
2. Kiểm tra firewall đã mở port 8080
3. Kiểm tra IP server đúng: `ipconfig` / `ifconfig`
4. Ping từ client đến server

### Vấn đề: Connection refused
**Giải pháp:**
- Server chưa chạy hoặc đã crash
- Firewall đang chặn port 8080
- IP address không đúng

### Vấn đề: Connection timeout
**Giải pháp:**
- Server và client không cùng mạng LAN
- Firewall đang chặn
- Router đang chặn kết nối

---

## 8. Kiểm tra Port đang được sử dụng

### Windows:
```powershell
netstat -an | findstr 8080
```

### Linux:
```bash
netstat -tuln | grep 8080
# hoặc
ss -tuln | grep 8080
```

Nếu thấy `0.0.0.0:8080` hoặc `:::8080`, server đang lắng nghe đúng.

---

## 9. Test Kết nối với Telnet

### Windows:
```powershell
telnet 192.168.1.100 8080
```

### Linux:
```bash
telnet 192.168.1.100 8080
```

Nếu kết nối thành công (không báo lỗi), port đã mở và server đang chạy.

---

## Lưu ý

- **Port 8080** là port mặc định, có thể thay đổi trong `common.h` (PORT)
- Server phải chạy trước khi client kết nối
- Đảm bảo server và client cùng mạng LAN hoặc có thể truy cập lẫn nhau
- Nếu dùng VM, khuyến nghị dùng **Bridged Network** mode




