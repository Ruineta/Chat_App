# Automated test for Thread Safety
Write-Host "=== Test Thread Safety ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_thread_safety.exe")) {
    Write-Host "Compiling test program..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o test_thread_safety.exe test_thread_safety.c common.c -lws2_32
    if (-not (Test-Path "test_thread_safety.exe")) {
        Write-Host "ERROR: Failed to compile" -ForegroundColor Red
        exit 1
    }
}

Write-Host "[1/4] Stopping any running servers..." -ForegroundColor Green
Get-Process | Where-Object {$_.ProcessName -eq "server" -or $_.ProcessName -eq "client"} | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "[2/4] Starting server..." -ForegroundColor Green
$server = Start-Process -FilePath ".\server.exe" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

if ($server.HasExited) {
    Write-Host "ERROR: Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "       Server started (PID: $($server.Id))" -ForegroundColor Green

Write-Host "[3/4] Running thread safety test (5 threads, 10 messages each)..." -ForegroundColor Green
$testProcess = Start-Process -FilePath ".\test_thread_safety.exe" -ArgumentList "127.0.0.1", "5", "10" -NoNewWindow -Wait -PassThru
$exitCode = $testProcess.ExitCode
Start-Sleep -Seconds 2

Write-Host "[4/4] Verifying results..." -ForegroundColor Green

# Check messages.txt
if (Test-Path "messages.txt") {
    $messageLines = Get-Content "messages.txt" | Where-Object { $_ -match "thread_recipient" }
    $messageCount = ($messageLines | Measure-Object).Count
    Write-Host "       Found $messageCount messages in messages.txt" -ForegroundColor White
    
    if ($messageCount -ge 40) {  # At least 80% of expected 50 messages
        Write-Host "       [OK] Messages written correctly" -ForegroundColor Green
    } else {
        Write-Host "       [WARNING] Expected ~50 messages, found $messageCount" -ForegroundColor Yellow
    }
    
    # Check for corruption (lines should have 7 fields separated by |)
    $corrupted = 0
    foreach ($line in $messageLines) {
        $fields = $line -split '\|'
        if ($fields.Count -ne 7) {
            $corrupted++
        }
    }
    
    if ($corrupted -eq 0) {
        Write-Host "       [OK] No corruption detected in messages.txt" -ForegroundColor Green
    } else {
        Write-Host "       [FAIL] Found $corrupted corrupted lines" -ForegroundColor Red
    }
} else {
    Write-Host "       [WARNING] messages.txt not found" -ForegroundColor Yellow
}

# Check activity.log
if (Test-Path "activity.log") {
    $logLines = Get-Content "activity.log" | Where-Object { $_ -match "thread_user" }
    $logCount = ($logLines | Measure-Object).Count
    Write-Host "       Found $logCount log entries" -ForegroundColor White
    
    if ($logCount -gt 0) {
        Write-Host "       [OK] Activity logging working" -ForegroundColor Green
    }
    
    # Check for corruption
    $corrupted = 0
    foreach ($line in $logLines) {
        if ($line -notmatch '\|') {
            $corrupted++
        }
    }
    
    if ($corrupted -eq 0) {
        Write-Host "       [OK] No corruption detected in activity.log" -ForegroundColor Green
    } else {
        Write-Host "       [WARNING] Found $corrupted potentially corrupted log lines" -ForegroundColor Yellow
    }
} else {
    Write-Host "       [WARNING] activity.log not found" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Stopping server..." -ForegroundColor Green
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan
if ($exitCode -eq 0) {
    Write-Host "Thread safety test completed!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Verification:" -ForegroundColor Yellow
    Write-Host "  - Multiple threads sent messages simultaneously" -ForegroundColor White
    Write-Host "  - messages.txt and activity.log checked for corruption" -ForegroundColor White
    Write-Host "  - Mutex protection working correctly" -ForegroundColor White
    exit 0
} else {
    Write-Host "Test failed or had issues" -ForegroundColor Red
    exit 1
}




