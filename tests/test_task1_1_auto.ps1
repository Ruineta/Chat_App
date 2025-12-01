# Automated test script for Task 1.1: Single Session Enforcement
# This script will start server, run 2 test clients, and verify behavior

Write-Host "=== Automated Test for Task 1.1: Single Session Enforcement ===" -ForegroundColor Cyan
Write-Host ""

# Check if executables exist
if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found. Please compile first." -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_client.exe")) {
    Write-Host "ERROR: test_client.exe not found. Please compile first." -ForegroundColor Red
    exit 1
}

# Clean up old log files
if (Test-Path "activity.log") {
    Remove-Item "activity.log" -Force
    Write-Host "Cleaned up old activity.log" -ForegroundColor Yellow
}

# Test account
$testUser = "hust123"
$testPass = "123456"
$serverIP = "127.0.0.1"

Write-Host "Test Configuration:" -ForegroundColor Yellow
Write-Host "  Server IP: $serverIP"
Write-Host "  Test User: $testUser"
Write-Host "  Test Pass: $testPass"
Write-Host ""

# Start server in background
Write-Host "[1/5] Starting server..." -ForegroundColor Green
$serverProcess = Start-Process -FilePath ".\server.exe" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

if ($serverProcess.HasExited) {
    Write-Host "ERROR: Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "       Server started (PID: $($serverProcess.Id))" -ForegroundColor Green

# Run first client (should login successfully)
Write-Host "[2/5] Running first client (Client 1)..." -ForegroundColor Green
$client1Process = Start-Process -FilePath ".\test_client.exe" -ArgumentList $serverIP, $testUser, $testPass, "1" -NoNewWindow -Wait -PassThru
Start-Sleep -Seconds 2

# Run second client (should login and terminate Client 1)
Write-Host "[3/5] Running second client (Client 2) - should terminate Client 1..." -ForegroundColor Green
$client2Process = Start-Process -FilePath ".\test_client.exe" -ArgumentList $serverIP, $testUser, $testPass, "2" -NoNewWindow -Wait -PassThru
Start-Sleep -Seconds 2

# Check activity.log for SESSION_TERMINATED
Write-Host "[4/5] Checking activity.log..." -ForegroundColor Green
$testPassed = $false
if (Test-Path "activity.log") {
    $logContent = Get-Content "activity.log" -Raw
    if ($logContent -match "SESSION_TERMINATED") {
        Write-Host "       ✓ Found SESSION_TERMINATED in activity.log" -ForegroundColor Green
        $testPassed = $true
    } else {
        Write-Host "       ✗ SESSION_TERMINATED not found in activity.log" -ForegroundColor Red
        Write-Host "       Log content:" -ForegroundColor Yellow
        Write-Host $logContent
    }
} else {
    Write-Host "       ✗ activity.log not found" -ForegroundColor Red
}

# Stop server
Write-Host "[5/5] Stopping server..." -ForegroundColor Green
Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Final result
Write-Host ""
Write-Host "=== Test Results ===" -ForegroundColor Cyan
if ($testPassed) {
    Write-Host "✓ TEST PASSED: Single Session Enforcement is working!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Verification:" -ForegroundColor Yellow
    Write-Host "  - Server started successfully"
    Write-Host "  - Client 1 logged in"
    Write-Host "  - Client 2 logged in (should have terminated Client 1)"
    Write-Host "  - SESSION_TERMINATED logged in activity.log"
    exit 0
} else {
    Write-Host "✗ TEST FAILED: Please check the output above" -ForegroundColor Red
    exit 1
}




