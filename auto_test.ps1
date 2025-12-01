# Auto Test Script - Tự động chạy server và client
# Script này sẽ start server và mở client để test

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  AUTO TEST - CHAT APPLICATION" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Kiểm tra executables
if (-not (Test-Path "build/server.exe")) {
    Write-Host "✗ server.exe not found! Run start_test.ps1 first." -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "build/client.exe")) {
    Write-Host "✗ client.exe not found! Run start_test.ps1 first." -ForegroundColor Red
    exit 1
}

Write-Host "Starting server in background..." -ForegroundColor Yellow
Start-Process -FilePath "build\server.exe" -WorkingDirectory "build" -WindowStyle Normal

Write-Host "Waiting 3 seconds for server to start..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

Write-Host "Starting client..." -ForegroundColor Yellow
Start-Process -FilePath "build\client.exe" -WorkingDirectory "build" -WindowStyle Normal

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  SERVER & CLIENT STARTED" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Instructions:" -ForegroundColor White
Write-Host "  1. Server window: Should show 'Waiting for clients...'" -ForegroundColor Gray
Write-Host "  2. Client window: Should show login menu" -ForegroundColor Gray
Write-Host ""
Write-Host "Quick Test:" -ForegroundColor Yellow
Write-Host "  - Register: 1 → username → password" -ForegroundColor Gray
Write-Host "  - Login: 2 → username → password" -ForegroundColor Gray
Write-Host "  - Add Friend: 4 → friend_username" -ForegroundColor Gray
Write-Host "  - Chat 1-1: 11 → friend_username → message" -ForegroundColor Gray
Write-Host ""
Write-Host "To stop: Close both windows or press Ctrl+C" -ForegroundColor Yellow
Write-Host ""

