# Task Plan: bugprone ~90件 clang-tidy 警告修正

Mode: implementation

## Goal
`bugprone-branch-clone`, `cppcoreguidelines-special-member-functions`, `cppcoreguidelines-owning-memory` の3カテゴリ警告を修正する。

## 実際の警告数（重複排除後）

| カテゴリ | ユニーク件数 | 対象 |
|---------|------------|------|
| `bugprone-branch-clone` | 8 | 分岐重複 |
| `special-member-functions` | ~30クラス | Rule of 5 |
| `owning-memory` | 18 | 生ポインタ所有 |

## Phases

### Phase 1: branch-clone (8件) — `complete`
各分岐を読み、バグか意図的かを判断して修正。

### Phase 2: special-member-functions (~30クラス) — `complete`
インターフェース/基底クラス → NS_DISALLOW_COPY_AND_MOVE + デフォルトコンストラクタ明示 + public: 復元。

### Phase 3: owning-memory (18件) — `complete`
ConsoleManager.cpp の raw new/delete → unique_ptr 化。
WindowsPlatformFile の OpenRead/OpenWrite 戻り値型を unique_ptr に変更。

### Phase 4: ビルド & テスト検証 — `complete`
ビルド成功、テスト 1294/1294 全パス。

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| NS_DISALLOW_COPY_AND_MOVE が private: に切替え、後続メンバーが private 化 | 1 | 全16箇所に `public:` を追加 |
| マクロが copy/move コンストラクタを宣言し暗黙デフォルトコンストラクタが消失 | 1 | 全インターフェースに `ClassName() = default;` を追加 |
| GenericPlatformSplash.h で macros.h 未インクルード | 1 | `#include "common/utility/macros.h"` を追加 |
