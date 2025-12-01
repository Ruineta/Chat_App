# Automated test for 3 new features
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  TEST NEW FEATURES" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_new_features.exe")) {
    Write-Host "Compiling test program..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o test_new_features.exe test_new_features.c common.c -lws2_32
    if (-not (Test-Path "test_new_features.exe")) {
        Write-Host "ERROR: Failed to compile" -ForegroundColor Red
        exit 1
    }
}

Write-Host "[1/3] Stopping any running servers..." -ForegroundColor Green
Get-Process | Where-Object {$_.ProcessName -eq "server" -or $_.ProcessName -eq "client"} | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "[2/3] Starting server..." -ForegroundColor Green
$server = Start-Process -FilePath ".\server.exe" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

if ($server.HasExited) {
    Write-Host "ERROR: Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "       Server started (PID: $($server.Id))" -ForegroundColor Green

Write-Host "[3/3] Running tests..." -ForegroundColor Green
Write-Host ""
$testProcess = Start-Process -FilePath ".\test_new_features.exe" -ArgumentList "127.0.0.1" -NoNewWindow -Wait -PassThru
$exitCode = $testProcess.ExitCode
Start-Sleep -Seconds 2

Write-Host ""
Write-Host "Stopping server..." -ForegroundColor Green
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  TEST COMPLETED" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if ($exitCode -eq 0) {
    Write-Host "All tests PASSED!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Features verified:" -ForegroundColor Yellow
    Write-Host "  - Friend Request Flow" -ForegroundColor White
    Write-Host "  - Last Seen Feature" -ForegroundColor White
    Write-Host "  - Sound Notification (code verified)" -ForegroundColor White
} else {
    Write-Host "Some tests FAILED. Check output above." -ForegroundColor Red
}

exit $exitCode
