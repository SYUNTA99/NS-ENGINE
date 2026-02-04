# サブ計画 4: FileOperationResult置き換え

## 目的

`FileOperationResult` 型を `NS::Result` に置き換え、書き込み・削除等の操作APIを統一する。

## 変更対象ファイル

- `source/engine/fs/file_system_types.h` - 型定義
- `source/engine/fs/file_system.h` - IWritableFileSystem インターフェース
- `source/engine/fs/host_file_system.h` - HostFileSystem
- `source/engine/fs/host_file_system.cpp` - 実装

## TODO

- [ ] IWritableFileSystem のメソッド署名を NS::Result に変更
- [ ] HostFileSystem の実装を更新
- [ ] 旧 FileOperationResult を削除
- [ ] 利用箇所を更新

## アプローチ

### 現在のAPI

```cpp
[[nodiscard]] FileOperationResult WriteFile(const std::string& path,
                                            const void* data, size_t size);
[[nodiscard]] FileOperationResult DeleteFile(const std::string& path);
[[nodiscard]] FileOperationResult CreateDirectory(const std::string& path);
```

### 新しいAPI

```cpp
/// ファイルに書き込む
[[nodiscard]] NS::Result WriteFile(const std::string& path,
                                    const void* data, size_t size);

/// ファイルを削除する
[[nodiscard]] NS::Result DeleteFile(const std::string& path);

/// ディレクトリを作成する
[[nodiscard]] NS::Result CreateDirectory(const std::string& path);
```

### 移行パターン

```cpp
// Before
auto result = fs.WriteFile("path", data, size);
if (!result.success) {
    Log(result.errorMessage());
    return;
}

// After
auto result = fs.WriteFile("path", data, size);
if (result.IsFailure()) {
    NS::Result::LogResultIfFailed(result, "WriteFile failed");
    return;
}
```

## 検証方法

- ファイル書き込みテストがパス
- ディレクトリ作成テストがパス
- エラー時に適切なResult値が返される

## 影響範囲

- IWritableFileSystem を実装する全クラス
- 書き込み操作を行う全コード

## 依存関係

- サブ計画1（FileSystemResult拡張）が完了していること
- サブ計画2（FileError変換関数）が完了していること
