# 調査結果: 独自Result型

## 調査日
2026-02-05

## 発見した独自Result型

### FileSystem モジュール

**ファイル**: `source/engine/fs/`

| 型 | ファイル | 説明 |
|----|----------|------|
| `FileError` | `file_error.h` | 独自エラー型（Code enum + nativeError + context） |
| `FileReadResult` | `file_system_types.h` | bool success + FileError + bytes |
| `FileOperationResult` | `file_system_types.h` | bool success + FileError |
| `AsyncReadState` | `file_system_types.h` | Pending/Running/Completed/Cancelled/Failed enum |

### FileError::Code の定義

```cpp
enum class Code {
    None,           // エラーなし
    NotFound,       // ファイル/ディレクトリが見つからない
    AccessDenied,   // アクセス権限がない
    InvalidPath,    // パス形式が不正
    InvalidMount,   // マウントが見つからない/不正
    DiskFull,       // ディスク容量不足
    AlreadyExists,  // 既に存在する
    NotEmpty,       // ディレクトリが空でない
    IsDirectory,    // ディレクトリに対してファイル操作を行った
    IsNotDirectory, // ファイルに対してディレクトリ操作を行った
    PathTooLong,    // パスが長すぎる
    ReadOnly,       // 読み取り専用
    Cancelled,      // 操作がキャンセルされた
    Unknown,        // 不明なエラー
};
```

## FileSystemResult.h との対応

| FileError::Code | FileSystemResult | 状態 |
|-----------------|------------------|------|
| None | ResultSuccess() | 既存 |
| NotFound | ResultNotFound() | 既存 |
| AccessDenied | ResultAccessDenied() | 既存 |
| InvalidPath | ResultPathInvalid() | 既存 |
| InvalidMount | **要追加** | - |
| DiskFull | ResultDiskFull() | 既存 |
| AlreadyExists | ResultAlreadyExists() | 既存 |
| NotEmpty | ResultDirectoryNotEmpty() | 既存 |
| IsDirectory | ResultIsDirectory() | 既存 |
| IsNotDirectory | **要追加** | - |
| PathTooLong | ResultPathTooLong() | 既存 |
| ReadOnly | ResultReadOnlyFileSystem() | 既存 |
| Cancelled | CommonResult::ResultOperationCancelled() | 既存 |
| Unknown | ResultUnknown() | 既存 |

## 対象外

### JobResult（状態enum）

`JobResult` は Pending/Success/Cancelled/Exception の**状態**を表すものであり、Result型（成功/失敗の二値）とは概念が異なる。置き換え対象外。

## 関連ファイル一覧

- `source/engine/fs/file_error.h`
- `source/engine/fs/file_error.cpp`
- `source/engine/fs/file_system_types.h`
- `source/engine/fs/file_system.h`
- `source/engine/fs/file_system_manager.h`
- `source/engine/fs/file_system_manager.cpp`
- `source/engine/fs/host_file_system.h`
- `source/engine/fs/host_file_system.cpp`
