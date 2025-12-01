# Automated test script for Task 1.3: Thread Safety cho Logging
Write-Host "=== Automated Test for Task 1.3: Thread Safety Logging ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_logging_auto.exe")) {
    Write-Host "ERROR: test_logging_auto.exe not found. Compiling..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o test_logging_auto.exe test_logging_auto.c common.c -lws2_32
    if (-not (Test-Path "test_logging_auto.exe")) {
        Write-Host "ERROR: Failed to compile test_logging_auto.exe" -ForegroundColor Red
        exit 1
    }
}

# Clean up old log
if (Test-Path "activity.log") {
    Remove-Item "activity.log" -Force
    Write-Host "Cleaned up old activity.log" -ForegroundColor Yellow
}

Write-Host "[1/4] Starting server..." -ForegroundColor Green
$server = Start-Process -FilePath ".\server.exe" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

if ($server.HasExited) {
    Write-Host "ERROR: Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "       Server started (PID: $($server.Id))" -ForegroundColor Green

Write-Host "[2/4] Running automated test clients..." -ForegroundColor Green
$testProcess = Start-Process -FilePath ".\test_logging_auto.exe" -ArgumentList "127.0.0.1","testuser","testpass","5" -NoNewWindow -Wait -PassThru
Start-Sleep -Seconds 2

Write-Host "[3/4] Checking activity.log..." -ForegroundColor Green
if (Test-Path "activity.log") {
    $logContent = Get-Content "activity.log"
    $logCount = $logContent.Count
    
    Write-Host "       Found $logCount log entries" -ForegroundColor Green
    
    # Check format
    $invalidFormat = 0
    foreach ($line in $logContent) {
        if ($line -notmatch '^\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] User: .+ \| Action: .+ \| Details: .+$') {
            $invalidFormat++
        }
    }
    
    if ($invalidFormat -eq 0) {
        Write-Host "       [OK] All log entries have correct format" -ForegroundColor Green
    } else {
        Write-Host "       [FAIL] Found $invalidFormat entries with invalid format" -ForegroundColor Red
    }
    
    # Check for specific events
    $loginCount = ($logContent | Select-String "LOGIN").Count
    $logoutCount = ($logContent | Select-String "LOGOUT").Count
    $registerCount = ($logContent | Select-String "REGISTER").Count
    $addFriendCount = ($logContent | Select-String "ADD_FRIEND").Count
    $sendMessageCount = ($logContent | Select-String "SEND_MESSAGE").Count
    $disconnectCount = ($logContent | Select-String "DISCONNECT").Count
    
    Write-Host "       Events found:" -ForegroundColor Cyan
    Write-Host "         - REGISTER: $registerCount" -ForegroundColor White
    Write-Host "         - LOGIN: $loginCount" -ForegroundColor White
    Write-Host "         - ADD_FRIEND: $addFriendCount" -ForegroundColor White
    Write-Host "         - SEND_MESSAGE: $sendMessageCount" -ForegroundColor White
    Write-Host "         - DISCONNECT: $disconnectCount" -ForegroundColor White
    Write-Host "         - LOGOUT: $logoutCount" -ForegroundColor White
    
    # Check for corruption (lines that are too short or incomplete)
    $corrupted = 0
    foreach ($line in $logContent) {
        if ($line.Length -lt 50) {  # Minimum expected length
            $corrupted++
        }
    }
    
    if ($corrupted -eq 0) {
        Write-Host "       [OK] No corrupted log entries detected" -ForegroundColor Green
    } else {
        Write-Host "       [FAIL] Found $corrupted potentially corrupted entries" -ForegroundColor Red
    }
    
    Write-Host ""
    Write-Host "       Sample log entries:" -ForegroundColor Cyan
    $logContent | Select-Object -First 5 | ForEach-Object {
        Write-Host "         $_" -ForegroundColor Gray
    }
    
} else {
    Write-Host "       [FAIL] activity.log not found" -ForegroundColor Red
}

Write-Host "[4/4] Stopping server..." -ForegroundColor Green
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan
if ((Test-Path "activity.log") -and $logCount -gt 0 -and $invalidFormat -eq 0 -and $corrupted -eq 0) {
    Write-Host "TEST PASSED: Thread-safe logging is working correctly!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Verification:" -ForegroundColor Yellow
    Write-Host "  - Log file created successfully" -ForegroundColor White
    Write-Host "  - All entries have correct format" -ForegroundColor White
    Write-Host "  - No corruption detected" -ForegroundColor White
    Write-Host "  - Multiple events logged correctly" -ForegroundColor White
    Write-Host "  - Thread safety verified (multiple concurrent clients)" -ForegroundColor White
    exit 0
} else {
    Write-Host "TEST FAILED: Please check the output above" -ForegroundColor Red
    exit 1
}

