# サブ計画 2: FileError置き換え

## 目的

`FileError` 型を `NS::Result` に置き換えるための変換ユーティリティを作成し、段階的に内部実装を移行する。

## 変更対象ファイル

- `source/engine/fs/file_error.h` - 変換関数追加
- `source/engine/fs/file_error.cpp` - 変換関数実装

## TODO

- [x] `FileError::ToResult()` メンバ関数を追加 — file_error.h L52, file_error.cpp L62-98
- [x] `FileError::FromResult(NS::Result)` 静的関数を追加（互換性用） — file_error.h L57, file_error.cpp L100-140
- [x] 変換テーブルを実装 — 全14コードの双方向マッピング完了
- [x] 単体テストを追加 — source/tests/engine/file_error_test.cpp（189行）

## アプローチ

### Phase 1: 変換関数の追加

```cpp
// file_error.h に追加
#include "common/result/ResultAll.h"

struct FileError {
    // 既存のメンバ...

    /// NS::Result に変換
    [[nodiscard]] NS::Result ToResult() const noexcept;

    /// NS::Result から FileError を生成（互換性用）
    [[nodiscard]] static FileError FromResult(NS::Result result) noexcept;
};
```

### Phase 2: 変換テーブル

```cpp
// file_error.cpp
NS::Result FileError::ToResult() const noexcept {
    using namespace NS::Result;
    switch (code) {
        case Code::None:         return ResultSuccess();
        case Code::NotFound:     return FileSystem::ResultNotFound();
        case Code::AccessDenied: return FileSystem::ResultAccessDenied();
        case Code::InvalidPath:  return FileSystem::ResultPathInvalid();
        case Code::InvalidMount: return FileSystem::ResultInvalidMount();
        // ... 他のマッピング
        default:                 return FileSystem::ResultUnknown();
    }
}
```

## 検証方法

- 全てのFileError::Codeが正しくNS::Resultにマッピングされること
- 往復変換（FileError → Result → FileError）で情報が保持されること

## 影響範囲

- FileError に依存する全てのコード（ただし互換性は維持）
