# Script để test UI Framework
# Usage: .\test_ui.ps1

Write-Host "========================================" -ForegroundColor Green
Write-Host "  UI FRAMEWORK - QUICK TEST" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

# Stop existing processes
Write-Host "Dừng các process đang chạy..." -ForegroundColor Yellow
Get-Process | Where-Object {$_.ProcessName -eq "server" -or $_.ProcessName -eq "client"} | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Compile
Write-Host "Compiling..." -ForegroundColor Yellow
Write-Host "  - common.c..." -ForegroundColor White
gcc -Wall -Wextra -std=c11 -c common.c -o common.o 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ❌ Lỗi compile common.c" -ForegroundColor Red
    exit 1
}

Write-Host "  - ui.c..." -ForegroundColor White
gcc -Wall -Wextra -std=c11 -c ui.c -o ui.o 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ❌ Lỗi compile ui.c" -ForegroundColor Red
    exit 1
}

Write-Host "  - client.c..." -ForegroundColor White
gcc -Wall -Wextra -std=c11 -c client.c -o client.o 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ❌ Lỗi compile client.c" -ForegroundColor Red
    exit 1
}

Write-Host "  - server.c..." -ForegroundColor White
gcc -Wall -Wextra -std=c11 -c server.c -o server.o 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ❌ Lỗi compile server.c" -ForegroundColor Red
    exit 1
}

# Link
Write-Host "Linking..." -ForegroundColor Yellow
Write-Host "  - server.exe..." -ForegroundColor White
gcc -Wall -Wextra -std=c11 -o server.exe server.o common.o -lws2_32 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ❌ Lỗi link server.exe" -ForegroundColor Red
    exit 1
}

Write-Host "  - client.exe..." -ForegroundColor White
gcc -Wall -Wextra -std=c11 -o client.exe client.o common.o ui.o -lws2_32 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ❌ Lỗi link client.exe" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "✅ Compile thành công!" -ForegroundColor Green
Write-Host ""

# Start server in background
Write-Host "Khởi động server..." -ForegroundColor Yellow
Start-Process -FilePath ".\server.exe" -WindowStyle Normal
Start-Sleep -Seconds 2

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  KIỂM TRA UI FRAMEWORK" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Server đã chạy. Bây giờ hãy:" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Mở terminal mới và chạy:" -ForegroundColor Yellow
Write-Host "   .\client.exe" -ForegroundColor White
Write-Host ""
Write-Host "2. Kiểm tra menu:" -ForegroundColor Yellow
Write-Host "   ✅ Menu có box style (╔ ═ ╗)" -ForegroundColor White
Write-Host "   ✅ Menu chia 3 nhóm rõ ràng" -ForegroundColor White
Write-Host "   ✅ Section headers có màu Cyan" -ForegroundColor White
Write-Host ""
Write-Host "3. Test input prompts:" -ForegroundColor Yellow
Write-Host "   - Chọn menu 1 (Register)" -ForegroundColor White
Write-Host "   - Kiểm tra: > Username: (có màu)" -ForegroundColor White
Write-Host "   - Kiểm tra: > Password: (có màu)" -ForegroundColor White
Write-Host ""
Write-Host "4. Test notifications:" -ForegroundColor Yellow
Write-Host "   - Register thành công → [✓] Success (màu Green)" -ForegroundColor White
Write-Host "   - Login sai → [✗] Error (màu Red)" -ForegroundColor White
Write-Host ""
Write-Host "5. Test validation:" -ForegroundColor Yellow
Write-Host "   - Chọn menu 5 (Send Message)" -ForegroundColor White
Write-Host "   - Để trống message → Error notification" -ForegroundColor White
Write-Host ""
Write-Host "6. Test UTF-8:" -ForegroundColor Yellow
Write-Host "   - Gửi message với emoji: 😀 🎉" -ForegroundColor White
Write-Host "   - Kiểm tra box characters hiển thị đúng" -ForegroundColor White
Write-Host ""
Write-Host "Xem chi tiết trong: UI_TEST_GUIDE.md" -ForegroundColor Cyan
Write-Host ""
Write-Host "Nhấn Enter để mở client..." -ForegroundColor Yellow
Read-Host

# Start client
Start-Process -FilePath ".\client.exe" -WindowStyle Normal

Write-Host ""
Write-Host "Client đã mở. Hãy kiểm tra UI!" -ForegroundColor Green
Write-Host ""
Write-Host "Để dừng server, nhấn Ctrl+C hoặc đóng cửa sổ server." -ForegroundColor Yellow


