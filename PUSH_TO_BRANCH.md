# Hướng Dẫn Push Code Lên Nhánh "truonggiang"

## Mục Tiêu
Tạo nhánh mới "truonggiang" và push code lên GitHub (KHÔNG push vào main).

## Các Bước Thực Hiện

### Bước 1: Khởi tạo Git (nếu chưa có)

```powershell
# Kiểm tra xem đã có git repo chưa
git status

# Nếu chưa có, khởi tạo
git init
```

### Bước 2: Thêm Remote Repository

```powershell
# Thêm remote (nếu chưa có)
git remote add origin https://github.com/Ruineta/Chat_App.git

# Hoặc nếu đã có remote nhưng sai URL, cập nhật:
git remote set-url origin https://github.com/Ruineta/Chat_App.git

# Kiểm tra remote
git remote -v
```

### Bước 3: Thêm Tất Cả Files

```powershell
# Thêm tất cả files
git add .

# Kiểm tra files sẽ commit
git status
```

### Bước 4: Commit Lần Đầu (nếu chưa commit)

```powershell
# Commit
git commit -m "Initial commit: Chat Application - Refactored structure"
```

### Bước 5: Tạo Nhánh "truonggiang" và Push

```powershell
# Tạo nhánh mới "truonggiang" từ nhánh hiện tại
git checkout -b truonggiang

# Push nhánh lên GitHub và set upstream
git push -u origin truonggiang
```

## Hoặc Nếu Đã Có Code Trên Main

Nếu repo đã có code trên main và bạn muốn tạo nhánh từ main:

```powershell
# Fetch code từ GitHub
git fetch origin

# Tạo nhánh mới từ main
git checkout -b truonggiang origin/main

# Hoặc nếu main ở local
git checkout -b truonggiang main

# Push nhánh lên
git push -u origin truonggiang
```

## Sau Khi Push

### Kiểm Tra Trên GitHub:
1. Vào: https://github.com/Ruineta/Chat_App
2. Click dropdown "main" ở góc trên bên trái
3. Bạn sẽ thấy nhánh "truonggiang"
4. Chọn nhánh "truonggiang" để xem code của bạn

### Lần Sau Muốn Push Tiếp:

```powershell
# Chuyển sang nhánh truonggiang (nếu đang ở nhánh khác)
git checkout truonggiang

# Thêm files mới/sửa đổi
git add .

# Commit
git commit -m "Your commit message"

# Push lên nhánh truonggiang
git push
```

## Lưu Ý Quan Trọng

✅ **Đúng:** Push vào nhánh `truonggiang`
❌ **Sai:** Push vào nhánh `main`

Sau khi push, code của bạn sẽ ở nhánh `truonggiang`, không ảnh hưởng đến `main`.

## Troubleshooting

### Lỗi: "fatal: not a git repository"
→ Chạy `git init` trước

### Lỗi: "remote origin already exists"
→ Dùng `git remote set-url origin https://github.com/Ruineta/Chat_App.git`

### Lỗi: "failed to push some refs"
→ Có thể cần pull trước: `git pull origin main --allow-unrelated-histories`

### Lỗi: "authentication failed"
→ Cần cấu hình GitHub credentials hoặc dùng Personal Access Token

