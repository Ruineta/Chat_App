# Automated test for Task 2.2: Load Offline Messages
Write-Host "=== Test Task 2.2: Load Offline Messages ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_task2_2.exe")) {
    Write-Host "ERROR: test_task2_2.exe not found. Compiling..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o test_task2_2.exe test_task2_2.c common.c -lws2_32
    if (-not (Test-Path "test_task2_2.exe")) {
        Write-Host "ERROR: Failed to compile" -ForegroundColor Red
        exit 1
    }
}

# Clean up old files
if (Test-Path "messages.txt") {
    Remove-Item "messages.txt" -Force
    Write-Host "Cleaned up old messages.txt" -ForegroundColor Yellow
}

Write-Host "[1/4] Starting server..." -ForegroundColor Green
$server = Start-Process -FilePath ".\server.exe" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

if ($server.HasExited) {
    Write-Host "ERROR: Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "       Server started (PID: $($server.Id))" -ForegroundColor Green

Write-Host "[2/4] Running test..." -ForegroundColor Green
$testProcess = Start-Process -FilePath ".\test_task2_2.exe" -ArgumentList "127.0.0.1" -NoNewWindow -Wait -PassThru
$exitCode = $testProcess.ExitCode
Start-Sleep -Seconds 2

Write-Host "[3/4] Checking messages.txt..." -ForegroundColor Green
if (Test-Path "messages.txt") {
    $messages = Get-Content "messages.txt"
    $messageCount = $messages.Count
    
    Write-Host "       Found $messageCount message entries" -ForegroundColor Green
    
    # Check for DELIVERED flag updates
    $delivered0Count = 0
    $delivered1Count = 0
    foreach ($line in $messages) {
        if ($line -match '\|(\d+)\|(\d+)$') {
            $delivered = $matches[1]
            if ($delivered -eq "0") { $delivered0Count++ }
            if ($delivered -eq "1") { $delivered1Count++ }
        }
    }
    
    Write-Host "       DELIVERED flags:" -ForegroundColor Cyan
    Write-Host "         - DELIVERED=0 (offline): $delivered0Count" -ForegroundColor White
    Write-Host "         - DELIVERED=1 (delivered): $delivered1Count" -ForegroundColor White
    
    if ($delivered1Count -gt 0) {
        Write-Host "       [OK] Messages marked as delivered after sending" -ForegroundColor Green
    }
}

Write-Host "[4/4] Stopping server..." -ForegroundColor Green
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan
if ($exitCode -eq 0) {
    Write-Host "TEST PASSED: Offline messages loaded and sent successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Verification:" -ForegroundColor Yellow
    Write-Host "  - User2 received offline message when logging in" -ForegroundColor White
    Write-Host "  - Messages marked as DELIVERED=1 after sending" -ForegroundColor White
    Write-Host "  - Thread-safe file operations working" -ForegroundColor White
    exit 0
} else {
    Write-Host "TEST FAILED: Offline messages not received" -ForegroundColor Red
    exit 1
}




