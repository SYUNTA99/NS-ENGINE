# サブ計画 3: FileReadResult置き換え

## 目的

`FileReadResult` 型を `NS::Result` + 出力パラメータに置き換え、APIを統一する。

## 変更対象ファイル

- `source/engine/fs/file_system_types.h` - 型定義
- `source/engine/fs/file_system.h` - IReadableFileSystem インターフェース
- `source/engine/fs/file_system_manager.h` - FileSystemManager
- `source/engine/fs/file_system_manager.cpp` - 実装
- `source/engine/fs/host_file_system.h` - HostFileSystem
- `source/engine/fs/host_file_system.cpp` - 実装

## TODO

- [ ] 新しいAPI署名を定義（NS::Result + outBytes）
- [ ] IReadableFileSystem に新メソッド追加
- [ ] HostFileSystem に新メソッド実装
- [ ] FileSystemManager に新メソッド追加
- [ ] 旧APIを[[deprecated]]でマーク
- [ ] 利用箇所を新APIに移行

## アプローチ

### 現在のAPI

```cpp
[[nodiscard]] FileReadResult ReadFile(const std::string& path);
```

### 新しいAPI

```cpp
/// ファイルを読み込む
/// @param path ファイルパス
/// @param outBytes 読み込んだデータの出力先
/// @return 成功時はResultSuccess()、失敗時はエラーResult
[[nodiscard]] NS::Result ReadFile(const std::string& path,
                                   std::vector<std::byte>& outBytes);

// 互換性のため旧APIも残す（deprecated）
[[deprecated("Use ReadFile(path, outBytes) instead")]]
[[nodiscard]] FileReadResult ReadFileLegacy(const std::string& path);
```

### 移行パターン

```cpp
// Before
auto result = fs.ReadFile("path");
if (!result.success) {
    Log(result.errorMessage());
    return;
}
ProcessData(result.bytes);

// After
std::vector<std::byte> bytes;
auto result = fs.ReadFile("path", bytes);
if (result.IsFailure()) {
    NS::Result::LogResultIfFailed(result, "ReadFile failed");
    return;
}
ProcessData(bytes);
```

## 検証方法

- 既存のファイル読み込みテストがパス
- Texture/Mesh/Shaderのロードが正常動作
- エラー時に適切なResult値が返される

## 影響範囲

- FileSystemを使用する全モジュール
- 段階的移行のため、旧APIは一時的に維持
