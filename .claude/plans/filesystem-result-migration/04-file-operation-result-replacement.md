# サブ計画 4: FileOperationResult置き換え

## 目的

`FileOperationResult` 型を `NS::Result` に置き換え、書き込み・削除等の操作APIを統一する。

## 変更対象ファイル

### コア（インターフェース・実装）

- `source/engine/fs/file_system_types.h` - FileOperationResult 型定義
- `source/engine/fs/file_system.h` - IWritableFileSystem インターフェース
- `source/engine/fs/host_file_system.h` - HostFileSystem 宣言
- `source/engine/fs/host_file_system.cpp` - HostFileSystem 実装

### 追加対象（元の計画で漏れていたもの）

- `source/engine/fs/host_file_system.cpp` — `makeError()` / `makeErrorFromLastError()` ヘルパーの移行

## 対象メソッド一覧（IWritableFileSystem 全8メソッド）

| クラス | メソッド | 現戻り値 | 備考 |
|--------|----------|---------|------|
| `IWritableFileSystem` | `createFile(path, size)` | `FileOperationResult` | ファイル作成 |
| `IWritableFileSystem` | `deleteFile(path)` | `FileOperationResult` | ファイル削除 |
| `IWritableFileSystem` | `renameFile(old, new)` | `FileOperationResult` | ファイルリネーム |
| `IWritableFileSystem` | `writeFile(path, data)` | `FileOperationResult` | ファイル書き込み |
| `IWritableFileSystem` | `createDirectory(path)` | `FileOperationResult` | ディレクトリ作成 |
| `IWritableFileSystem` | `deleteDirectory(path)` | `FileOperationResult` | ディレクトリ削除 |
| `IWritableFileSystem` | `deleteDirectoryRecursively(path)` | `FileOperationResult` | ディレクトリ再帰削除 |
| `IWritableFileSystem` | `renameDirectory(old, new)` | `FileOperationResult` | ディレクトリリネーム |

すべて戻り値を `NS::Result` に変更する。

## TODO

### API定義

- [ ] `IWritableFileSystem` の全8メソッドの戻り値を `NS::Result` に変更
- [ ] `HostFileSystem` の対応する override を更新

### ヘルパー関数の移行

- [ ] `HostFileSystem::makeError(code)` → `makeResult(code)` に移行（内部で `ToResult()` を利用）
- [ ] `HostFileSystem::makeErrorFromLastError()` → `makeResultFromLastError()` に移行（ログ出力 + 抽象コード）

### 実装

- [ ] `HostFileSystem` の全8メソッド実装を `NS::Result` 返却に更新
- [ ] 各メソッド内の `FileError` 生成を `NS::Result` 生成に置き換え

### 外部利用箇所の移行

- [ ] `FileOperationResult` を使用している全箇所を検索・移行

### 型の整理

- [ ] `FileOperationResult` 構造体を `[[deprecated]]` マーク or 削除
- [ ] `file_system_types.h` から不要になった型を整理

## `HostFileSystem` ヘルパーの移行

### 現在の実装

```cpp
// host_file_system.cpp
static FileError makeError(FileError::Code code) noexcept;
static FileError makeErrorFromLastError() noexcept;  // GetLastError() → FileError::Code マッピング
```

`makeErrorFromLastError()` は Windows API の `GetLastError()` を以下のようにマッピング:
- `ERROR_FILE_NOT_FOUND` / `ERROR_PATH_NOT_FOUND` → `NotFound`
- `ERROR_ACCESS_DENIED` → `AccessDenied`
- `ERROR_ALREADY_EXISTS` / `ERROR_FILE_EXISTS` → `AlreadyExists`
- `ERROR_DISK_FULL` → `DiskFull`
- `ERROR_DIR_NOT_EMPTY` → `NotEmpty`
- `ERROR_INVALID_NAME` / `ERROR_BAD_PATHNAME` → `InvalidPath`
- その他 → `Unknown`

### 移行後の実装

```cpp
// host_file_system.cpp
static NS::Result makeResult(FileError::Code code) noexcept
{
    // FileError::ToResult() の変換テーブルを再利用
    return FileError{code}.ToResult();
}

static NS::Result makeResultFromLastError() noexcept
{
    DWORD lastError = ::GetLastError();

    // デバッグ用にネイティブエラーコードをログ出力
    LOG_DEBUG("[HostFileSystem] Native error: {}", lastError);

    switch (lastError) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return NS::FileSystem::ResultNotFound();
        case ERROR_ACCESS_DENIED:
            return NS::FileSystem::ResultAccessDenied();
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return NS::FileSystem::ResultAlreadyExists();
        case ERROR_DISK_FULL:
            return NS::FileSystem::ResultDiskFull();
        case ERROR_DIR_NOT_EMPTY:
            return NS::FileSystem::ResultDirectoryNotEmpty();
        case ERROR_INVALID_NAME:
        case ERROR_BAD_PATHNAME:
            return NS::FileSystem::ResultPathInvalid();
        default:
            LOG_WARNING("[HostFileSystem] Unknown native error: {}", lastError);
            return NS::FileSystem::ResultUnknownError();
    }
}
```

## アプローチ

### 現在のAPI

```cpp
[[nodiscard]] FileOperationResult WriteFile(const std::string& path,
                                            const void* data, size_t size);
[[nodiscard]] FileOperationResult DeleteFile(const std::string& path);
[[nodiscard]] FileOperationResult CreateDirectory(const std::string& path);
// ... 全8メソッド同様
```

### 新しいAPI

```cpp
[[nodiscard]] NS::Result writeFile(const std::string& path,
                                    const void* data, size_t size);
[[nodiscard]] NS::Result deleteFile(const std::string& path);
[[nodiscard]] NS::Result createDirectory(const std::string& path);
// ... 全8メソッド同様
```

### 移行パターン

```cpp
// Before
auto result = fs.writeFile("path", data, size);
if (!result.success) {
    LOG_ERROR("Write failed: {}", result.error.message());
    return;
}

// After
NS::Result result = fs.writeFile("path", data, size);
if (result.IsFailure()) {
    LOG_ERROR("Write failed: {}", NS::ResultToString(result));
    return;
}
```

## テストケース

- [ ] `writeFile` 成功時: `ResultSuccess()`
- [ ] `writeFile` ディスク満杯時: `ResultDiskFull()`
- [ ] `deleteFile` 未存在時: `ResultNotFound()` / `ResultPathNotFound()`
- [ ] `createDirectory` 既存時: `ResultAlreadyExists()`
- [ ] `deleteDirectory` 非空時: `ResultDirectoryNotEmpty()`
- [ ] `renameFile` アクセス拒否時: `ResultAccessDenied()`
- [ ] 全8メソッドが `NS::Result` を返すこと
- [ ] `makeResultFromLastError()` が Windows エラーコードを正しくマッピングすること
- [ ] 不明なネイティブエラー時に `ResultUnknownError()` + ログ出力されること

## 検証方法

- ファイル書き込みテストがパス
- ディレクトリ操作テストがパス
- エラー時に適切なResult値が返される
- ネイティブエラーコードがログに出力されること（デバッグビルド）

## 影響範囲

- `IWritableFileSystem` を実装する全クラス（現在は `HostFileSystem` のみ）
- 書き込み操作を行う全コード
- `FileOperationResult` は移行完了後に削除

## 依存関係

- サブ計画1（FileSystemResult拡張）が完了していること ✅
- サブ計画2（FileError変換関数）が完了していること ✅
- サブ計画3（FileReadResult置き換え）と並行実施可能（独立した型）
