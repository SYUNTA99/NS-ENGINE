---
paths:
  - "Source/**"
  - ".claude/plans/**"
---

# 命名規則

## ファイル・ディレクトリ

| 種類 | パターン | 例 |
|------|----------|-----|
| ソース | `PascalCase.h/.cpp` | `World.h`, `JobSystem.cpp` |
| テスト | `〜Test.cpp` | `EcsTest.cpp` |
| ディレクトリ | `PascalCase/` | `Source/Engine/Ecs/` |

## 名前空間

**namespace NS::X` (C++17入れ子宣言) は使わない**
```cpp
namespace NS { namespace ModuleName {   // 公開API
namespace NS { namespace Private { namespace ModuleName {  // 内部実装
```

### コード記述

| ファイル | `using namespace` | 型の参照 |
|----------|-------------------|----------|
| .h | 禁止 | 完全修飾 |
| .cpp | 自身のnamespaceのみ可 | 他は完全修飾 |
| テスト | 自由 | 自由 |

## クラス・構造体

| 種類 | パターン | 例 |
|------|----------|-----|
| クラス | `PascalCase` | `World`, `GraphicsDevice` |
| インターフェース | `I〜` | `ISystem`, `IAllocator` |
| コンポーネント | `PascalCase` | `Transform`, `Velocity`, `Sprite` |
| タグ | `PascalCase` | `Player`, `Dead` |
| システム | `〜System` | `MovementSystem` |
| マネージャー | `〜Manager` | `TextureManager` |
| GPU状態 | `〜State` | `BlendState` |
| ビルダー | `〜Builder` | `PrefabBuilder` |

## 関数

| 種類 | パターン | 例 |
|------|----------|-----|
| 通常 | `PascalCase` | `CreateActor()`, `Update()` |
| 取得 | `Get〜` | `GetPosition()` |
| 設定 | `Set〜` | `SetPosition()` |
| 判定 | `Is/Has/Can〜` | `IsAlive()`, `HasComponent()` |
| 生成 | `Create/Make〜` | `CreateTexture()` |
| コールバック | `On〜` | `OnUpdate()`, `OnDestroy()` |

## 変数

| 種類 | パターン | 例 |
|------|----------|-----|
| クラスメンバ | `m_camelCase` | `m_device`, `m_count` |
| 構造体メンバ | `camelCase` | `position`, `rotation` |
| 静的メンバ | `s_camelCase` | `s_instance` |
| グローバル | `g_camelCase` | `g_engine` |
| ローカル | `camelCase` | `actor`, `index` |
| 出力引数 | `out〜` | `outResult` |
| 定数 | `kPascalCase` | `kMaxCount` |
| constexpr | `kPascalCase` | `kChunkSize`, `kInvalidId` |

## マクロ

- 全大文字 + アンダースコア: `LIKE_THIS`
- エンジン共通: `NS_` プレフィックス
- モジュール固有: `ECS_`, `GFX_` など

## 列挙型

| 種類 | パターン | 例 |
|------|----------|-----|
| 型名 | `PascalCase` | `JobPriority`, `LogLevel` |
| 値 | `PascalCase` | `High`, `Normal`, `Low` |
| スコープ | `enum class` 必須 | `enum class JobPriority` |

## 型エイリアス

| 種類 | パターン | 例 |
|------|----------|-----|
| ID | `〜Id` | `ActorId`, `ArchetypeId` |
| ポインタ | `〜Ptr` | `TexturePtr` |
| 参照 | `〜Ref` | `ComponentRef<T>` |
| トレイト | `is_〜_v` | `is_component_v<T>` |

## 禁止

- ハンガリアン記法: `iCount`, `bEnabled`
- 先頭アンダースコア: `_count`
- 略語: `cnt`, `mgr`, `idx`, `num`, `buf`, `ptr`, `arr`, `tmp`
