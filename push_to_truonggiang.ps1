# Script tự động push code lên nhánh "truonggiang"
# Chạy script này để tự động thực hiện tất cả các bước

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  PUSH CODE LÊN NHÁNH 'truonggiang'" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Bước 1: Kiểm tra git repository
Write-Host "[1/6] Checking git repository..." -ForegroundColor Yellow
if (-not (Test-Path ".git")) {
    Write-Host "  Initializing git repository..." -ForegroundColor Gray
    git init
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  ✗ Failed to initialize git" -ForegroundColor Red
        exit 1
    }
    Write-Host "  ✓ Git repository initialized" -ForegroundColor Green
} else {
    Write-Host "  ✓ Git repository exists" -ForegroundColor Green
}
Write-Host ""

# Bước 2: Kiểm tra và thêm remote
Write-Host "[2/6] Checking remote repository..." -ForegroundColor Yellow
$remoteExists = git remote get-url origin 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  Adding remote origin..." -ForegroundColor Gray
    git remote add origin https://github.com/Ruineta/Chat_App.git
    Write-Host "  ✓ Remote added" -ForegroundColor Green
} else {
    if ($remoteExists -ne "https://github.com/Ruineta/Chat_App.git") {
        Write-Host "  Updating remote URL..." -ForegroundColor Gray
        git remote set-url origin https://github.com/Ruineta/Chat_App.git
        Write-Host "  ✓ Remote URL updated" -ForegroundColor Green
    } else {
        Write-Host "  ✓ Remote already configured" -ForegroundColor Green
    }
}
Write-Host ""

# Bước 3: Thêm files
Write-Host "[3/6] Adding files..." -ForegroundColor Yellow
git add .
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ Failed to add files" -ForegroundColor Red
    exit 1
}
Write-Host "  ✓ Files added" -ForegroundColor Green
Write-Host ""

# Bước 4: Commit (nếu có thay đổi)
Write-Host "[4/6] Checking for changes to commit..." -ForegroundColor Yellow
$status = git status --porcelain
if ($status) {
    Write-Host "  Committing changes..." -ForegroundColor Gray
    git commit -m "Update: Chat Application - Refactored structure and features"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  ✗ Failed to commit" -ForegroundColor Red
        exit 1
    }
    Write-Host "  ✓ Changes committed" -ForegroundColor Green
} else {
    Write-Host "  ✓ No changes to commit" -ForegroundColor Green
}
Write-Host ""

# Bước 5: Tạo nhánh truonggiang
Write-Host "[5/6] Creating branch 'truonggiang'..." -ForegroundColor Yellow
$currentBranch = git branch --show-current 2>$null
if ($currentBranch -ne "truonggiang") {
    # Kiểm tra xem nhánh đã tồn tại chưa
    $branchExists = git branch --list truonggiang
    if ($branchExists) {
        Write-Host "  Branch 'truonggiang' already exists, switching..." -ForegroundColor Gray
        git checkout truonggiang
    } else {
        Write-Host "  Creating new branch 'truonggiang'..." -ForegroundColor Gray
        git checkout -b truonggiang
    }
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  ✗ Failed to create/switch branch" -ForegroundColor Red
        exit 1
    }
    Write-Host "  ✓ Switched to branch 'truonggiang'" -ForegroundColor Green
} else {
    Write-Host "  ✓ Already on branch 'truonggiang'" -ForegroundColor Green
}
Write-Host ""

# Bước 6: Push lên GitHub
Write-Host "[6/6] Pushing to GitHub..." -ForegroundColor Yellow
Write-Host "  Pushing branch 'truonggiang' to origin..." -ForegroundColor Gray
git push -u origin truonggiang
if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  ✅ PUSH THÀNH CÔNG!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Code đã được push lên nhánh 'truonggiang'" -ForegroundColor Cyan
    Write-Host "Xem tại: https://github.com/Ruineta/Chat_App/tree/truonggiang" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Lần sau muốn push tiếp:" -ForegroundColor White
    Write-Host "  git add ." -ForegroundColor Gray
    Write-Host "  git commit -m 'Your message'" -ForegroundColor Gray
    Write-Host "  git push" -ForegroundColor Gray
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "  ✗ PUSH THẤT BẠI" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "Có thể cần:" -ForegroundColor Yellow
    Write-Host "  1. Xác thực GitHub (username/password hoặc token)" -ForegroundColor Gray
    Write-Host "  2. Kiểm tra kết nối internet" -ForegroundColor Gray
    Write-Host "  3. Chạy thủ công: git push -u origin truonggiang" -ForegroundColor Gray
    Write-Host ""
}

