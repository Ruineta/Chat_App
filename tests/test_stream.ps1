# Quick test script for Task 1.2: Stream Handling
Write-Host "=== Quick Test for Stream Handling ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "This will test that send_all() and recv_all() work correctly" -ForegroundColor Yellow
Write-Host ""

# Check if server and client exist
if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "client.exe")) {
    Write-Host "ERROR: client.exe not found" -ForegroundColor Red
    exit 1
}

Write-Host "Manual Test Steps:" -ForegroundColor Green
Write-Host ""
Write-Host "1. Open Terminal 1 and run:" -ForegroundColor White
Write-Host "   .\server.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "2. Open Terminal 2 and run:" -ForegroundColor White
Write-Host "   .\client.exe" -ForegroundColor Cyan
Write-Host "   - Option 2: Login with hust123 / 123456" -ForegroundColor Gray
Write-Host "   - Keep this terminal open" -ForegroundColor Gray
Write-Host ""
Write-Host "3. Open Terminal 3 and run:" -ForegroundColor White
Write-Host "   .\client.exe" -ForegroundColor Cyan
Write-Host "   - Option 1: Register new user (e.g., testuser / testpass)" -ForegroundColor Gray
Write-Host "   - Option 4: Add Friend -> hust123" -ForegroundColor Gray
Write-Host "   - Option 5: Send Message -> hust123" -ForegroundColor Gray
Write-Host "   - Enter a test message" -ForegroundColor Gray
Write-Host ""
Write-Host "4. Check Terminal 2:" -ForegroundColor White
Write-Host "   - Should receive the message completely" -ForegroundColor Gray
Write-Host "   - Message should be displayed in full" -ForegroundColor Gray
Write-Host ""
Write-Host "5. Test with long message:" -ForegroundColor White
Write-Host "   - From Terminal 3, send a long message (copy-paste a paragraph)" -ForegroundColor Gray
Write-Host "   - Check Terminal 2: Message should be received completely" -ForegroundColor Gray
Write-Host ""
Write-Host "Expected Result:" -ForegroundColor Yellow
Write-Host "  - All messages are sent/received completely" -ForegroundColor White
Write-Host "  - No data loss or truncation" -ForegroundColor White
Write-Host "  - send_all() and recv_all() handle partial transfers correctly" -ForegroundColor White
Write-Host ""
Write-Host "If all tests pass, Task 1.2 is working correctly!" -ForegroundColor Green




