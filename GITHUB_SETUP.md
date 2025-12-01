# Hướng Dẫn Push Lên GitHub - Dự Án Môn Học

## Bước 1: Khởi tạo Git Repository

```powershell
# Khởi tạo git repository
git init
```

## Bước 2: Thêm Tất Cả Files

```powershell
# Thêm tất cả files (bao gồm data/, logs/, docs/)
git add .

# Kiểm tra files sẽ được commit
git status
```

## Bước 3: Commit

```powershell
# Commit lần đầu
git commit -m "Initial commit: Chat Application - Network Programming Project"
```

## Bước 4: Tạo Repository trên GitHub

1. Đăng nhập GitHub
2. Click "New repository"
3. Đặt tên: `Chat_App` hoặc tên bạn muốn
4. **KHÔNG** check "Initialize with README" (vì đã có README)
5. Click "Create repository"

## Bước 5: Kết nối và Push

```powershell
# Thêm remote (thay <your-username> và <repo-name>)
git remote add origin https://github.com/<your-username>/<repo-name>.git

# Push lên GitHub
git branch -M main
git push -u origin main
```

## Files Sẽ Được Push

✅ **Source Code:**
- `src/*.c` - Source files
- `include/*.h` - Header files

✅ **Data & Logs:**
- `data/account.txt` - User accounts
- `data/messages.txt` - Message history
- `logs/activity.log` - Activity logs

✅ **Documentation:**
- `README.md` - Main documentation
- `docs/` - Tất cả documentation files
- `TESTING_GUIDE.md` - Test guide
- `QUICK_TEST.md` - Quick test guide

✅ **Configuration:**
- `Makefile` - Build configuration
- `.gitignore` - Git ignore rules

✅ **Scripts:**
- `start_test.ps1` - Setup script
- `auto_test.ps1` - Auto test script
- `tests/` - Tất cả test files

## Files Sẽ BỊ IGNORE (không push)

❌ `build/` - Build artifacts (compile tự động)
❌ `*.o` - Object files
❌ `.vscode/` - IDE settings
❌ `backup_v1/` - Backup directory

## Lưu Ý

- **Tất cả data và logs** sẽ được push (dự án môn học, không cần bảo mật)
- **Tất cả documentation** sẽ được push
- Đồng đội có thể clone và chạy ngay

## Sau Khi Push

Đồng đội có thể:
1. Clone repository: `git clone <repo-url>`
2. Chạy `make` để compile
3. Đọc `README.md` để biết cách sử dụng
4. Xem `TESTING_GUIDE.md` để test
