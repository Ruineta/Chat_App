# Quick test script for Task 1.3: Thread Safety cho Logging
Write-Host "=== Test Task 1.3: Thread Safety cho Logging ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "client.exe")) {
    Write-Host "ERROR: client.exe not found" -ForegroundColor Red
    exit 1
}

# Clean up old log
if (Test-Path "activity.log") {
    Remove-Item "activity.log" -Force
    Write-Host "Cleaned up old activity.log" -ForegroundColor Yellow
}

Write-Host "Manual Test Steps:" -ForegroundColor Green
Write-Host ""
Write-Host "=== Test 1: Basic Logging ===" -ForegroundColor Yellow
Write-Host "1. Open Terminal 1: .\server.exe" -ForegroundColor White
Write-Host "2. Open Terminal 2: .\client.exe" -ForegroundColor White
Write-Host "   - Option 2: Login (hust123 / 123456)" -ForegroundColor Gray
Write-Host "   - Option 4: Add Friend (thêm một user)" -ForegroundColor Gray
Write-Host "   - Option 5: Send Message" -ForegroundColor Gray
Write-Host "   - Option 17: Disconnect" -ForegroundColor Gray
Write-Host "3. Check log: Get-Content activity.log" -ForegroundColor White
Write-Host ""
Write-Host "Expected:" -ForegroundColor Cyan
Write-Host "  - Format: [TIMESTAMP] User: USERNAME | Action: ACTION | Details: DETAILS" -ForegroundColor White
Write-Host "  - Events: LOGIN, ADD_FRIEND, SEND_MESSAGE, DISCONNECT" -ForegroundColor White
Write-Host ""
Write-Host "=== Test 2: Thread Safety (Multiple Clients) ===" -ForegroundColor Yellow
Write-Host "1. Keep server running" -ForegroundColor White
Write-Host "2. Open Terminal 2, 3, 4: .\client.exe" -ForegroundColor White
Write-Host "   - Each terminal: Register -> Login -> Add Friend -> Send Message -> Disconnect" -ForegroundColor Gray
Write-Host "   - Do actions quickly, almost simultaneously" -ForegroundColor Gray
Write-Host "3. Check log: Get-Content activity.log" -ForegroundColor White
Write-Host ""
Write-Host "Expected:" -ForegroundColor Cyan
Write-Host "  - All log entries are complete" -ForegroundColor White
Write-Host "  - No corruption or truncated lines" -ForegroundColor White
Write-Host "  - Format correct for all entries" -ForegroundColor White
Write-Host ""
Write-Host "=== Test 3: SESSION_TERMINATED and LOGOUT ===" -ForegroundColor Yellow
Write-Host "1. Terminal 2: Login with hust123 / 123456" -ForegroundColor White
Write-Host "2. Terminal 3: Login with hust123 / 123456 (same user)" -ForegroundColor White
Write-Host "3. Check: Get-Content activity.log | Select-String 'SESSION_TERMINATED'" -ForegroundColor White
Write-Host "4. Disconnect one client" -ForegroundColor White
Write-Host "5. Check: Get-Content activity.log | Select-String 'LOGOUT'" -ForegroundColor White
Write-Host ""
Write-Host "Expected:" -ForegroundColor Cyan
Write-Host "  - SESSION_TERMINATED logged when old session terminated" -ForegroundColor White
Write-Host "  - LOGOUT logged when user disconnects" -ForegroundColor White
Write-Host ""
Write-Host "=== Quick Check Commands ===" -ForegroundColor Yellow
Write-Host "View all logs: Get-Content activity.log" -ForegroundColor White
Write-Host "Count entries: (Get-Content activity.log).Count" -ForegroundColor White
Write-Host "Check specific events: Get-Content activity.log | Select-String 'LOGIN|LOGOUT|DISCONNECT'" -ForegroundColor White
Write-Host ""
Write-Host "If all tests pass, Task 1.3 is working correctly!" -ForegroundColor Green




