# Test script for Task 1.1
Write-Host "=== Testing Task 1.1: Single Session Enforcement ===" -ForegroundColor Cyan

if (-not (Test-Path "server.exe")) {
    Write-Host "ERROR: server.exe not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "test_client.exe")) {
    Write-Host "ERROR: test_client.exe not found" -ForegroundColor Red
    exit 1
}

if (Test-Path "activity.log") {
    Remove-Item "activity.log" -Force
}

$testUser = "hust123"
$testPass = "123456"
$serverIP = "127.0.0.1"

Write-Host "Starting server..." -ForegroundColor Green
$server = Start-Process -FilePath ".\server.exe" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

if ($server.HasExited) {
    Write-Host "Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "Running Client 1 (background)..." -ForegroundColor Green
$client1 = Start-Process -FilePath ".\test_client.exe" -ArgumentList $serverIP,$testUser,$testPass,"1" -NoNewWindow -PassThru
Start-Sleep -Seconds 3

Write-Host "Running Client 2 (should terminate Client 1)..." -ForegroundColor Green
Start-Process -FilePath ".\test_client.exe" -ArgumentList $serverIP,$testUser,$testPass,"2" -NoNewWindow -Wait
Start-Sleep -Seconds 3

# Stop client 1 if still running
if (-not $client1.HasExited) {
    Stop-Process -Id $client1.Id -Force -ErrorAction SilentlyContinue
}
Start-Sleep -Seconds 2

Write-Host "Checking activity.log..." -ForegroundColor Green
$testPassed = $false
if (Test-Path "activity.log") {
    $log = Get-Content "activity.log" -Raw
    if ($log -match "SESSION_TERMINATED") {
        Write-Host "TEST PASSED: SESSION_TERMINATED found" -ForegroundColor Green
        $testPassed = $true
    } else {
        Write-Host "TEST FAILED: SESSION_TERMINATED not found" -ForegroundColor Red
        Write-Host "Log content:" -ForegroundColor Yellow
        Write-Host $log
    }
} else {
    Write-Host "TEST FAILED: activity.log not found" -ForegroundColor Red
}

Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

if ($testPassed) {
    Write-Host "=== Test PASSED ===" -ForegroundColor Green
    exit 0
} else {
    Write-Host "=== Test FAILED ===" -ForegroundColor Red
    exit 1
}

