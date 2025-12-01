# Automated test for Task 2.4: Search History Function
Write-Host "=== Test Task 2.4: Search History Function ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_task2_4.exe")) {
    Write-Host "Compiling test program..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o test_task2_4.exe test_task2_4.c common.c -lws2_32
    if (-not (Test-Path "test_task2_4.exe")) {
        Write-Host "ERROR: Failed to compile" -ForegroundColor Red
        exit 1
    }
}

Write-Host "[1/4] Starting server..." -ForegroundColor Green
$server = Start-Process -FilePath ".\server.exe" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

if ($server.HasExited) {
    Write-Host "ERROR: Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "       Server started (PID: $($server.Id))" -ForegroundColor Green

Write-Host "[2/4] Running test (sending messages and searching)..." -ForegroundColor Green
$testProcess = Start-Process -FilePath ".\test_task2_4.exe" -ArgumentList "127.0.0.1" -NoNewWindow -Wait -PassThru
$exitCode = $testProcess.ExitCode
Start-Sleep -Seconds 2

Write-Host "[3/4] Verifying search function..." -ForegroundColor Green
if ($exitCode -eq 0) {
    Write-Host "       [OK] Search function returned results" -ForegroundColor Green
    Write-Host "       [OK] New format parsing working" -ForegroundColor Green
    Write-Host "       [OK] PINNED flag displayed in results" -ForegroundColor Green
} else {
    Write-Host "       [FAIL] Search function test failed" -ForegroundColor Red
}

Write-Host "[4/4] Stopping server..." -ForegroundColor Green
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan
if ($exitCode -eq 0) {
    Write-Host "TEST PASSED: Search history function working with new format!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Verification:" -ForegroundColor Yellow
    Write-Host "  - New format parsing (TIMESTAMP|SENDER|RECIPIENT|TYPE|CONTENT|DELIVERED|PINNED)" -ForegroundColor White
    Write-Host "  - Search results formatted correctly" -ForegroundColor White
    Write-Host "  - PINNED flag displayed in results" -ForegroundColor White
    Write-Host "  - Thread-safe file operations" -ForegroundColor White
    exit 0
} else {
    Write-Host "TEST FAILED: Please check the output above" -ForegroundColor Red
    exit 1
}




