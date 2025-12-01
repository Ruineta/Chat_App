# Automated test for Phase 3: UI/UX Improvements
Write-Host "=== Test Phase 3: UI/UX Improvements ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_phase3.exe")) {
    Write-Host "Compiling test program..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o test_phase3.exe test_phase3.c common.c -lws2_32
    if (-not (Test-Path "test_phase3.exe")) {
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

Write-Host "[2/4] Running test (Friends List with Online/Offline Status)..." -ForegroundColor Green
$testProcess = Start-Process -FilePath ".\test_phase3.exe" -ArgumentList "127.0.0.1" -NoNewWindow -Wait -PassThru
$exitCode = $testProcess.ExitCode
Start-Sleep -Seconds 2

Write-Host "[3/4] Verifying improvements..." -ForegroundColor Green
if ($exitCode -eq 0) {
    Write-Host "       [OK] Friends list shows online/offline status" -ForegroundColor Green
    Write-Host "       [OK] Format: 'Friends List:\n  - USERNAME [ONLINE/OFFLINE]'" -ForegroundColor Green
} else {
    Write-Host "       [FAIL] Test failed" -ForegroundColor Red
}

Write-Host "[4/4] Stopping server..." -ForegroundColor Green
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan
if ($exitCode -eq 0) {
    Write-Host "TEST PASSED: Phase 3 UI/UX improvements working!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Completed tasks:" -ForegroundColor Yellow
    Write-Host "  - Task 3.1: Message format with timestamp [HH:MM:SS]" -ForegroundColor White
    Write-Host "  - Task 3.2: Friends list with [ONLINE]/[OFFLINE] status" -ForegroundColor White
    Write-Host "  - Task 3.3: Fixed sleep bug (sleep(100000) -> usleep(100000))" -ForegroundColor White
    exit 0
} else {
    Write-Host "TEST FAILED: Please check the output above" -ForegroundColor Red
    exit 1
}




