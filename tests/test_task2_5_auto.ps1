# Automated test for Task 2.5: Block User Logic
Write-Host "=== Test Task 2.5: Block User Logic ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_task2_5.exe")) {
    Write-Host "Compiling test program..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o test_task2_5.exe test_task2_5.c common.c -lws2_32
    if (-not (Test-Path "test_task2_5.exe")) {
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

Write-Host "[2/4] Running test (UserA blocks UserB, UserB tries to send message)..." -ForegroundColor Green
$testProcess = Start-Process -FilePath ".\test_task2_5.exe" -ArgumentList "127.0.0.1" -NoNewWindow -Wait -PassThru
$exitCode = $testProcess.ExitCode
Start-Sleep -Seconds 2

Write-Host "[3/4] Verifying block logic..." -ForegroundColor Green
if ($exitCode -eq 0) {
    Write-Host "       [OK] Block logic working correctly" -ForegroundColor Green
    Write-Host "       [OK] Only checks if recipient blocked sender" -ForegroundColor Green
    Write-Host "       [OK] Error message: 'You are blocked by this user'" -ForegroundColor Green
} else {
    Write-Host "       [FAIL] Block logic test failed" -ForegroundColor Red
}

Write-Host "[4/4] Stopping server..." -ForegroundColor Green
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan
if ($exitCode -eq 0) {
    Write-Host "TEST PASSED: Block user logic working correctly!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Verification:" -ForegroundColor Yellow
    Write-Host "  - Only checks if recipient blocked sender" -ForegroundColor White
    Write-Host "  - Correct error message: 'You are blocked by this user'" -ForegroundColor White
    Write-Host "  - Blocked user cannot send message" -ForegroundColor White
    exit 0
} else {
    Write-Host "TEST FAILED: Please check the output above" -ForegroundColor Red
    exit 1
}




