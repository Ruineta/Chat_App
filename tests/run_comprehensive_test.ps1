# Comprehensive Test Suite Runner
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  COMPREHENSIVE TEST SUITE" -ForegroundColor Cyan
Write-Host "  Chat Application - All Features" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    Write-Host "Please compile server first: gcc -o server.exe server.c common.c -lws2_32" -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path "comprehensive_test.exe")) {
    Write-Host "Compiling comprehensive test suite..." -ForegroundColor Yellow
    gcc -Wall -Wextra -std=c11 -o comprehensive_test.exe comprehensive_test.c common.c -lws2_32
    if (-not (Test-Path "comprehensive_test.exe")) {
        Write-Host "ERROR: Failed to compile test suite" -ForegroundColor Red
        exit 1
    }
}

# Stop any running server/client
Write-Host "[1/3] Stopping any running servers/clients..." -ForegroundColor Green
Get-Process | Where-Object {$_.ProcessName -eq "server" -or $_.ProcessName -eq "client" -or $_.ProcessName -eq "comprehensive_test"} | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Clean up old test files
if (Test-Path "test_*.txt") {
    Remove-Item "test_*.txt" -ErrorAction SilentlyContinue
}

Write-Host "[2/3] Starting server..." -ForegroundColor Green
$server = Start-Process -FilePath ".\server.exe" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

if ($server.HasExited) {
    Write-Host "ERROR: Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "       Server started (PID: $($server.Id))" -ForegroundColor Green
Write-Host "[3/3] Running comprehensive test suite..." -ForegroundColor Green
Write-Host ""

$testProcess = Start-Process -FilePath ".\comprehensive_test.exe" -ArgumentList "127.0.0.1" -NoNewWindow -Wait -PassThru
$exitCode = $testProcess.ExitCode

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
} else {
    Write-Host "Some tests FAILED. Check output above for details." -ForegroundColor Red
}

exit $exitCode




