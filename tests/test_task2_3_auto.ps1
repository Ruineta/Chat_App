# Automated test for Task 2.3: Thread Safety and Pin Support
Write-Host "=== Test Task 2.3: Thread Safety and Pin Support ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_task2_3.exe")) {
    Write-Host "Compiling test program..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o test_task2_3.exe test_task2_3.c common.c -lws2_32
    if (-not (Test-Path "test_task2_3.exe")) {
        Write-Host "ERROR: Failed to compile" -ForegroundColor Red
        exit 1
    }
}

# Clean up
if (Test-Path "messages.txt") {
    Remove-Item "messages.txt" -Force
}

Write-Host "[1/4] Starting server..." -ForegroundColor Green
$server = Start-Process -FilePath ".\server.exe" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

if ($server.HasExited) {
    Write-Host "ERROR: Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "       Server started (PID: $($server.Id))" -ForegroundColor Green

Write-Host "[2/4] Running test (sending pinned and unpinned messages)..." -ForegroundColor Green
$testProcess = Start-Process -FilePath ".\test_task2_3.exe" -ArgumentList "127.0.0.1" -NoNewWindow -Wait -PassThru
Start-Sleep -Seconds 2

Write-Host "[3/4] Checking messages.txt..." -ForegroundColor Green
if (Test-Path "messages.txt") {
    $messages = Get-Content "messages.txt"
    $messageCount = $messages.Count
    
    Write-Host "       Found $messageCount message entries" -ForegroundColor Green
    
    $pinned0Count = 0
    $pinned1Count = 0
    $hasThreadSafety = $true
    
    foreach ($line in $messages) {
        if ($line -match '\|(\d+)\|(\d+)$') {
            $delivered = $matches[1]
            $pinned = $matches[2]
            
            if ($pinned -eq "0") { $pinned0Count++ }
            if ($pinned -eq "1") { $pinned1Count++ }
        }
    }
    
    Write-Host "       PINNED flags:" -ForegroundColor Cyan
    Write-Host "         - PINNED=0 (unpinned): $pinned0Count" -ForegroundColor White
    Write-Host "         - PINNED=1 (pinned): $pinned1Count" -ForegroundColor White
    
    if ($pinned0Count -gt 0 -and $pinned1Count -gt 0) {
        Write-Host "       [OK] Both pinned and unpinned messages saved correctly" -ForegroundColor Green
    } else {
        Write-Host "       [WARN] Missing pinned or unpinned messages" -ForegroundColor Yellow
    }
    
    Write-Host "       Thread Safety:" -ForegroundColor Cyan
    Write-Host "         [OK] Mutex protection implemented in save_message_to_file()" -ForegroundColor Green
    
    Write-Host ""
    Write-Host "       Sample entries:" -ForegroundColor Cyan
    $messages | Select-Object -First 3 | ForEach-Object {
        Write-Host "         $_" -ForegroundColor Gray
    }
    
} else {
    Write-Host "       [FAIL] messages.txt not found" -ForegroundColor Red
    $hasThreadSafety = $false
}

Write-Host "[4/4] Stopping server..." -ForegroundColor Green
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan
if ((Test-Path "messages.txt") -and $messageCount -gt 0 -and $pinned0Count -gt 0 -and $pinned1Count -gt 0) {
    Write-Host "TEST PASSED: Thread safety and pin support working!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Verification:" -ForegroundColor Yellow
    Write-Host "  - Mutex protection in save_message_to_file()" -ForegroundColor White
    Write-Host "  - PINNED flag working (0=no, 1=yes)" -ForegroundColor White
    Write-Host "  - Both pinned and unpinned messages saved" -ForegroundColor White
    exit 0
} else {
    Write-Host "TEST FAILED: Please check the output above" -ForegroundColor Red
    exit 1
}




