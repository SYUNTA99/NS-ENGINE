# サブ計画 3: FileReadResult置き換え

## 目的

`FileReadResult` 型を `NS::Result` + 出力パラメータに置き換え、APIを統一する。

## 変更対象ファイル

### コア（インターフェース・実装）

- `source/engine/fs/file_system_types.h` - FileReadResult 型定義、AsyncReadHandle
- `source/engine/fs/file_system.h` - IReadableFileSystem / IFileHandle インターフェース
- `source/engine/fs/file_system_manager.h` - FileSystemManager ファサード
- `source/engine/fs/file_system_manager.cpp` - FileSystemManager 実装
- `source/engine/fs/host_file_system.h` - HostFileSystem 宣言
- `source/engine/fs/host_file_system.cpp` - HostFileSystem 実装

### 追加対象（元の計画で漏れていたもの）

- `source/engine/fs/memory_file_system.h` - MemoryFileSystem 宣言（IReadableFileSystem::read() を override）
- `source/engine/fs/memory_file_system.cpp` - MemoryFileSystem 実装
- `source/engine/fs/file_system_functions.h` - ReadFile() / ReadFileAsText() / ReadFileAsChars() インラインラッパー

### 外部利用箇所

- `source/engine/texture/texture_manager.cpp` - FileReadResult を直接使用（L360-387）

## 対象メソッド一覧

`FileReadResult` を返す/使う全メソッド:

| クラス | メソッド | 現シグネチャ | 備考 |
|--------|----------|-------------|------|
| `IFileHandle` | `read(size_t)` | `FileReadResult` | チャンク読み込み |
| `IReadableFileSystem` | `read(path)` | `FileReadResult` | 純粋仮想 |
| `IReadableFileSystem` | `readAsText(path)` | `std::string` | デフォルト実装、内部で `read()` 呼出 |
| `IReadableFileSystem` | `readAsChars(path)` | `std::vector<char>` | デフォルト実装、内部で `read()` 呼出 |
| `IReadableFileSystem` | `readAsync(path)` | `AsyncReadHandle` | `std::future<FileReadResult>` を包む |
| `IReadableFileSystem` | `readAsync(path, callback)` | `void` | callback が `const FileReadResult&` を受ける |
| `HostFileSystem` | `read(path)` | `FileReadResult` | override |
| `MemoryFileSystem` | `read(path)` | `FileReadResult` | override |
| `FileSystemManager` | `ReadFile(mountPath)` | `FileReadResult` | ファサード |
| `file_system_functions.h` | `ReadFile(mountPath)` | `FileReadResult` | インラインラッパー |
| `file_system_functions.h` | `ReadFileAsText(mountPath)` | `std::string` | インラインラッパー |
| `file_system_functions.h` | `ReadFileAsChars(mountPath)` | `std::vector<char>` | インラインラッパー |
| `AsyncReadHandle` | `get()` | `FileReadResult` | future から取得 |
| `AsyncReadHandle` | `getFor(timeout)` | `std::optional<FileReadResult>` | タイムアウト付き |
| `TextureManager` | `LoadTexture2D()` 等 | — | FileReadResult.bytes を使用 |

## TODO

### API定義

- [ ] `IReadableFileSystem::read(path)` を `NS::Result read(path, std::vector<std::byte>& outBytes)` に変更
- [ ] `IFileHandle::read(size_t)` を `NS::Result read(size_t, std::vector<std::byte>& outBytes)` に変更
- [ ] `readAsText()` / `readAsChars()` のデフォルト実装を新 `read()` に合わせて更新
- [ ] `FileSystemManager::ReadFile()` の署名を更新

### 実装

- [ ] `HostFileSystem::read()` を新署名で実装
- [ ] `HostFileHandle::read()` を新署名で実装
- [ ] `MemoryFileSystem::read()` を新署名で実装
- [ ] `MemoryFileHandle::read()` を新署名で実装
- [ ] `file_system_functions.h` の `ReadFile()` を新署名に更新

### 外部利用箇所の移行

- [ ] `TextureManager` の FileReadResult 使用箇所を新APIに移行
- [ ] 他の FileReadResult 利用箇所を検索・移行

### 型の整理

- [ ] `FileReadResult` 構造体を `[[deprecated]]` マーク or 削除
- [ ] `file_system_types.h` から不要になった型を整理

## 設計決定

### `nativeError` / `context` 情報の喪失

`NS::Result` は32bitのエラーコードのみを格納する。`FileError` が持つ以下のフィールドは `NS::Result` に移行すると喪失する:

- `nativeError` (int32_t) — OS固有エラーコード（例: `GetLastError()` の値）
- `context` (std::string) — エラー発生時のパス情報等

**決定**: 喪失を許容し、ログ出力で対応する。

- `HostFileSystem::makeErrorFromLastError()` 内で `nativeError` をログに出力してから `NS::Result` の抽象コードに変換
- `context`（パス情報）は呼び出し元が保持している前提（呼び出し元でログ出力）
- 理由: `NS::Result` の32bit制約に追加データを含めると型の一貫性が崩れる。詳細なエラー情報が必要な場面はデバッグ時のみであり、ログで十分

### `AsyncReadHandle` は今回の移行対象外

`AsyncReadHandle` は `std::future<FileReadResult>` を内包しており、この型変更は非同期パターン全体の再設計が必要になる。

**決定**: 今回のスコープから除外する。

- 理由: `FileReadResult` を単純に `NS::Result` + `outBytes` に分離すると、非同期の場合に `bytes` と `Result` を別々に future で返す設計が必要になり、複雑性が大きい
- 将来的に `AsyncReadHandle<std::vector<std::byte>>` 等のテンプレート化、または `AsyncResult<T>` パターンの導入を検討
- 移行期間中は `AsyncReadHandle` 内部で旧 `FileReadResult` を使い続ける

### `readAsText()` / `readAsChars()` は自動的に移行

これらは `IReadableFileSystem` のデフォルト実装で、内部で `read()` を呼び出している。`read()` の署名変更に伴いデフォルト実装を更新すれば、override している箇所がない限り自動的に移行される。

## アプローチ

### 現在のAPI

```cpp
// IReadableFileSystem
[[nodiscard]] virtual FileReadResult read(const std::string& path) = 0;

// IFileHandle
[[nodiscard]] virtual FileReadResult read(size_t maxBytes) = 0;
```

### 新しいAPI

```cpp
// IReadableFileSystem
[[nodiscard]] virtual NS::Result read(const std::string& path,
                                       std::vector<std::byte>& outBytes) = 0;

// IFileHandle
[[nodiscard]] virtual NS::Result read(size_t maxBytes,
                                       std::vector<std::byte>& outBytes) = 0;
```

### 移行パターン

```cpp
// Before
auto result = fs.read("path");
if (!result.success) {
    LOG_ERROR("Read failed: {}", result.errorMessage());
    return;
}
ProcessData(result.bytes);

// After
std::vector<std::byte> bytes;
NS::Result result = fs.read("path", bytes);
if (result.IsFailure()) {
    LOG_ERROR("Read failed: {}", NS::ResultToString(result));
    return;
}
ProcessData(bytes);
```

### TextureManager の移行パターン

```cpp
// Before (texture_manager.cpp L360-387)
FileReadResult fileResult;
if (path.find(":/") != std::string::npos) {
    fileResult = FileSystemManager::Get().ReadFile(path);
} else {
    fileResult = fileSystem_->read(path);
}
if (!fileResult.success || fileResult.bytes.empty()) {
    LOG_ERROR("[TextureManager] ファイルの読み込みに失敗: " + path);
    return TextureHandle::Invalid();
}
loader->Load(fileResult.bytes.data(), fileResult.bytes.size(), ...);

// After
std::vector<std::byte> bytes;
NS::Result readResult;
if (path.find(":/") != std::string::npos) {
    readResult = FileSystemManager::Get().ReadFile(path, bytes);
} else {
    readResult = fileSystem_->read(path, bytes);
}
if (readResult.IsFailure() || bytes.empty()) {
    LOG_ERROR("[TextureManager] ファイルの読み込みに失敗: {}", path);
    return TextureHandle::Invalid();
}
loader->Load(bytes.data(), bytes.size(), ...);
```

## テストケース

- [ ] `IReadableFileSystem::read()` が `NS::Result` を返すこと
- [ ] 成功時: `ResultSuccess()` + `outBytes` にデータ格納
- [ ] ファイル未存在時: `ResultNotFound()` / `ResultPathNotFound()`
- [ ] アクセス拒否時: `ResultAccessDenied()`
- [ ] `readAsText()` が内部で新APIを使い正常動作すること
- [ ] `readAsChars()` が内部で新APIを使い正常動作すること
- [ ] `MemoryFileSystem::read()` が新APIに準拠すること
- [ ] `TextureManager` が新APIで正常にテクスチャロードできること
- [ ] `HostFileHandle::read()` がチャンク読み込みで正しくデータを返すこと
- [ ] `MemoryFileHandle::read()` がチャンク読み込みで正しくデータを返すこと
- [ ] `file_system_functions.h` の `ReadFile()` が新APIで動作すること

## 検証方法

- 既存のファイル読み込みテストがパス
- Texture/Mesh/Shaderのロードが正常動作
- エラー時に適切なResult値が返される
- MemoryFileSystem のテストがパス

## 影響範囲

- FileSystemを使用する全モジュール
- 段階的移行のため、`AsyncReadHandle` は旧APIを維持
- `readAsText()` / `readAsChars()` はデフォルト実装更新で自動移行

## 依存関係

- サブ計画1（FileSystemResult拡張）が完了していること ✅
- サブ計画2（FileError変換関数）が完了していること ✅
