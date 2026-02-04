# Findings: 独自Result型の調査

## 調査目的

`common/result` のResult型を使っていないエンジン機能で、独自のResult型やエラー処理パターンを使っている箇所を特定し、置き換え計画を立てる。

---

## 発見した独自Result型

### 1. FileSystem モジュール（高優先度）

**ファイル**: `source/engine/fs/`

| 型 | ファイル | 説明 |
|----|----------|------|
| `FileError` | `file_error.h` | 独自エラー型（Code enum + nativeError + context） |
| `FileReadResult` | `file_system_types.h` | bool success + FileError + bytes |
| `FileOperationResult` | `file_system_types.h` | bool success + FileError |
| `AsyncReadState` | `file_system_types.h` | Pending/Running/Completed/Cancelled/Failed enum |

**FileError::Code の定義**:
- None, NotFound, AccessDenied, InvalidPath, InvalidMount
- DiskFull, AlreadyExists, NotEmpty, IsDirectory, IsNotDirectory
- PathTooLong, ReadOnly, Cancelled, Unknown

**評価**: `common/result/Module/FileSystemResult.h` で対応するエラーコードが既に定義済み。直接マッピング可能。

---

### 2. JobSystem モジュール（低優先度）

**ファイル**: `source/engine/core/job_system.h`

| 型 | 説明 |
|----|------|
| `JobResult` | Pending/Success/Cancelled/Exception のenum class (uint8_t) |

**用途**: ジョブの実行状態を表す（Result型とは異なる概念）
- `Pending = 0` - 未完了（待機中/実行中）
- `Success = 1` - 正常完了
- `Cancelled = 2` - キャンセルされた
- `Exception = 3` - 例外が発生した

**評価**: これは「状態」を表すenumであり、`NS::Result`型への置き換えは**不適切**。
- `Pending`は成功でも失敗でもない中間状態
- ジョブシステムの設計思想と合わない
- **置き換え対象外**とする

---

## bool を返す関数パターン（将来の移行候補）

多くのエンジン関数が `bool` を返してエラーを表現している:

### Graphics モジュール
- `SpriteBatch::Initialize()` → bool
- `MeshBatch::Initialize()` → bool
- `MeshBatch::CreateShaders()` → bool
- `MeshBatch::CreateConstantBuffers()` → bool

### Platform モジュール
- `Renderer::Initialize()` → bool
- `Window::CreateWindowInternal()` → bool
- `Application::Initialize()` → bool

### Texture モジュール
- `ITextureLoader::Load()` → bool
- `WICTextureLoader::Load()` → bool
- `DDSTextureLoader::Load()` → bool

### Engine Core
- `Engine::Initialize()` → bool
- `Engine::CreateSingletons()` → bool
- `Engine::InitializeSubsystems()` → bool

**評価**: これらは将来的にResult型に移行することで、詳細なエラー情報を提供できる。ただし影響範囲が大きいため段階的に実施。

---

## ECS モジュール

独自のResult型は使用していない。主に:
- `bool` 戻り値（`IsAlive()`, `HasComponent()`等）
- アサーションによるエラー検出

**評価**: ECS内部のエラー処理は主にデバッグ時のアサーション。
`common/result/Module/EcsResult.h` は外部APIのエラー報告用として既に定義済み。

---

## 置き換え優先度

| 優先度 | 対象 | 理由 |
|--------|------|------|
| **高** | FileSystem | 独自のResult型が最も発達。直接マッピング可能 |
| **中** | Graphics初期化 | bool→Resultで詳細エラー情報を提供 |
| **中** | Texture読み込み | FileSystemと連携、エラー伝搬が必要 |
| **低** | Platform初期化 | 影響範囲が広い、段階的移行 |
| **対象外** | JobResult | 状態を表すenum、Result型の概念と異なる |

---

## 技術的考慮事項

### FileError → NS::Result マッピング

```cpp
// 現在
FileError::Code::NotFound → FileSystemResult::ResultNotFound()

// common/result/Module/FileSystemResult.h の既存定義と対応
FileError::Code      →  FileSystemResult
-----------------------------------------
None                 →  ResultSuccess()
NotFound             →  ResultNotFound()
AccessDenied         →  ResultAccessDenied()
InvalidPath          →  ResultPathInvalid()
InvalidMount         →  (新規追加が必要)
DiskFull             →  ResultDiskFull()
AlreadyExists        →  ResultAlreadyExists()
NotEmpty             →  ResultDirectoryNotEmpty()
IsDirectory          →  ResultIsDirectory()
IsNotDirectory       →  (新規追加が必要: ResultIsNotDirectory)
PathTooLong          →  ResultPathTooLong()
ReadOnly             →  ResultReadOnlyFileSystem()
Cancelled            →  ResultOperationCancelled() (CommonResultから)
Unknown              →  ResultUnknown()
```

### FileReadResult/FileOperationResult の移行

```cpp
// 現在
struct FileReadResult {
    bool success;
    FileError error;
    std::vector<std::byte> bytes;
};

// 移行後: Result<T>パターンまたは分離
NS::Result ReadFile(path, outBytes);  // Resultでエラー、成功時にoutBytesにデータ
```

---

## 次のステップ

1. FileSystemモジュールの移行計画を詳細化
2. FileSystemResult.h に不足しているエラーコードを追加
3. FileError → NS::Result 変換ユーティリティを作成
4. FileReadResult/FileOperationResult を段階的に置き換え
