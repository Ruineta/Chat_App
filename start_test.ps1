# Quick Test Script - Tự động setup và chạy test
# Chạy script này để test nhanh dự án

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  CHAT APPLICATION - QUICK TEST" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Tạo thư mục cần thiết
Write-Host "[1/5] Creating directories..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path "build","data","logs" -Force | Out-Null
Write-Host "  ✓ Directories ready" -ForegroundColor Green
Write-Host ""

# Step 2: Compile project
Write-Host "[2/5] Compiling project..." -ForegroundColor Yellow
$compileSuccess = $true

# Compile common
Write-Host "  Compiling common.c..." -ForegroundColor Gray
gcc -Wall -Wextra -std=c11 -Iinclude -c src/common.c -o build/common.o 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) { $compileSuccess = $false }

# Compile server
Write-Host "  Compiling server.c..." -ForegroundColor Gray
gcc -Wall -Wextra -std=c11 -Iinclude -c src/server.c -o build/server.o 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) { $compileSuccess = $false }

# Compile client
Write-Host "  Compiling client.c..." -ForegroundColor Gray
gcc -Wall -Wextra -std=c11 -Iinclude -c src/client.c -o build/client.o 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) { $compileSuccess = $false }

# Compile UI
Write-Host "  Compiling ui.c..." -ForegroundColor Gray
gcc -Wall -Wextra -std=c11 -Iinclude -c src/ui.c -o build/ui.o 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) { $compileSuccess = $false }

# Link server
Write-Host "  Linking server.exe..." -ForegroundColor Gray
gcc -Wall -Wextra -std=c11 -Iinclude -o build/server.exe build/server.o build/common.o -lws2_32 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) { $compileSuccess = $false }

# Link client
Write-Host "  Linking client.exe..." -ForegroundColor Gray
gcc -Wall -Wextra -std=c11 -Iinclude -o build/client.exe build/client.o build/common.o build/ui.o -lws2_32 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) { $compileSuccess = $false }

if ($compileSuccess) {
    Write-Host "  ✓ Compilation successful!" -ForegroundColor Green
} else {
    Write-Host "  ✗ Compilation failed!" -ForegroundColor Red
    exit 1
}
Write-Host ""

# Step 3: Verify executables
Write-Host "[3/5] Verifying executables..." -ForegroundColor Yellow
if (Test-Path "build/server.exe" -and Test-Path "build/client.exe") {
    Write-Host "  ✓ server.exe ready" -ForegroundColor Green
    Write-Host "  ✓ client.exe ready" -ForegroundColor Green
} else {
    Write-Host "  ✗ Executables not found!" -ForegroundColor Red
    exit 1
}
Write-Host ""

# Step 4: Check data directories
Write-Host "[4/5] Checking data directories..." -ForegroundColor Yellow
if (Test-Path "data" -and Test-Path "logs") {
    Write-Host "  ✓ data/ directory ready" -ForegroundColor Green
    Write-Host "  ✓ logs/ directory ready" -ForegroundColor Green
} else {
    Write-Host "  ✗ Directories missing!" -ForegroundColor Red
    exit 1
}
Write-Host ""

# Step 5: Instructions
Write-Host "[5/5] Setup complete!" -ForegroundColor Yellow
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  READY TO TEST" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Để test, mở 2 terminal windows:" -ForegroundColor White
Write-Host ""
Write-Host "Terminal 1 - Start Server:" -ForegroundColor Yellow
Write-Host "  cd build" -ForegroundColor Gray
Write-Host "  .\server.exe" -ForegroundColor Gray
Write-Host ""
Write-Host "Terminal 2 - Start Client:" -ForegroundColor Yellow
Write-Host "  cd build" -ForegroundColor Gray
Write-Host "  .\client.exe" -ForegroundColor Gray
Write-Host ""
Write-Host "Hoặc chạy tự động:" -ForegroundColor White
Write-Host "  .\auto_test.ps1" -ForegroundColor Cyan
Write-Host ""

