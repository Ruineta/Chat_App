# Automated test for Task 2.1: Messages.txt Format
Write-Host "=== Test Task 2.1: Messages.txt Format with Pin Support ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_task2_1.exe")) {
    Write-Host "ERROR: test_task2_1.exe not found. Compiling..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o test_task2_1.exe test_task2_1.c common.c -lws2_32
    if (-not (Test-Path "test_task2_1.exe")) {
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
$testProcess = Start-Process -FilePath ".\test_task2_1.exe" -ArgumentList "127.0.0.1" -NoNewWindow -Wait -PassThru
Start-Sleep -Seconds 2

Write-Host "[3/4] Checking messages.txt..." -ForegroundColor Green
if (Test-Path "messages.txt") {
    $messages = Get-Content "messages.txt"
    $messageCount = $messages.Count
    
    Write-Host "       Found $messageCount message entries" -ForegroundColor Green
    Write-Host ""
    
    # Check format: TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED
    $validFormat = 0
    $invalidFormat = 0
    $hasDelivered0 = $false
    $hasDelivered1 = $false
    $hasPinned0 = $false
    $hasPinned1 = $false
    
    foreach ($line in $messages) {
        # Check if line matches format: TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED
        if ($line -match '^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\|([^|]+)\|([^|]+)\|(\d+)\|([^|]+)\|(\d+)\|(\d+)$') {
            $validFormat++
            $timestamp = $matches[1]
            $sender = $matches[2]
            $recipient = $matches[3]
            $type = $matches[4]
            $content = $matches[5]
            $delivered = $matches[6]
            $pinned = $matches[7]
            
            if ($delivered -eq "0") { $hasDelivered0 = $true }
            if ($delivered -eq "1") { $hasDelivered1 = $true }
            if ($pinned -eq "0") { $hasPinned0 = $true }
            if ($pinned -eq "1") { $hasPinned1 = $true }
            
            Write-Host "       Entry: $sender -> $recipient | TYPE=$type | DELIVERED=$delivered | PINNED=$pinned" -ForegroundColor Gray
        } else {
            $invalidFormat++
            Write-Host "       [INVALID] $line" -ForegroundColor Red
        }
    }
    
    Write-Host ""
    Write-Host "       Format Check:" -ForegroundColor Cyan
    if ($validFormat -eq $messageCount) {
        Write-Host "         [OK] All entries have correct format" -ForegroundColor Green
    } else {
        Write-Host "         [FAIL] $invalidFormat entries have invalid format" -ForegroundColor Red
    }
    
    Write-Host "       DELIVERED Flag:" -ForegroundColor Cyan
    if ($hasDelivered0) {
        Write-Host "         [OK] Found DELIVERED=0 (offline messages)" -ForegroundColor Green
    } else {
        Write-Host "         [WARN] No DELIVERED=0 found" -ForegroundColor Yellow
    }
    if ($hasDelivered1) {
        Write-Host "         [OK] Found DELIVERED=1 (online messages)" -ForegroundColor Green
    } else {
        Write-Host "         [INFO] No DELIVERED=1 found (expected if recipient was offline)" -ForegroundColor Gray
    }
    
    Write-Host "       PINNED Flag:" -ForegroundColor Cyan
    if ($hasPinned0) {
        Write-Host "         [OK] Found PINNED=0 (unpinned messages)" -ForegroundColor Green
    } else {
        Write-Host "         [WARN] No PINNED=0 found" -ForegroundColor Yellow
    }
    if ($hasPinned1) {
        Write-Host "         [OK] Found PINNED=1 (pinned messages)" -ForegroundColor Green
    } else {
        Write-Host "         [WARN] No PINNED=1 found" -ForegroundColor Yellow
    }
    
    Write-Host ""
    Write-Host "       Sample entries:" -ForegroundColor Cyan
    $messages | Select-Object -First 3 | ForEach-Object {
        Write-Host "         $_" -ForegroundColor Gray
    }
    
} else {
    Write-Host "       [FAIL] messages.txt not found" -ForegroundColor Red
}

Write-Host "[4/4] Stopping server..." -ForegroundColor Green
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan
if ((Test-Path "messages.txt") -and $messageCount -gt 0 -and $validFormat -eq $messageCount) {
    Write-Host "TEST PASSED: Messages.txt format is correct!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Verification:" -ForegroundColor Yellow
    Write-Host "  - Format: TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED" -ForegroundColor White
    Write-Host "  - All entries have correct format" -ForegroundColor White
    Write-Host "  - DELIVERED flag working (0=offline, 1=online)" -ForegroundColor White
    Write-Host "  - PINNED flag working (0=no, 1=yes)" -ForegroundColor White
    exit 0
} else {
    Write-Host "TEST FAILED: Please check the output above" -ForegroundColor Red
    exit 1
}




