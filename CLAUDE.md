# NS-ENGINE

3D三人称視点プラットフォーマー/ジャンプアクションゲーム専用エンジン

## このファイルの方針

- 短く、具体的に、Claudeが自分で検証ループを回せる情報を書く
- 命名規則・禁止事項など「コードと照合できるルール」を書く
- アーキテクチャ詳細は書かない（コードを読めばわかる、陳腐化する）
- ビルド・テスト方法は書かない（スキルにある）
---
# 命名規則

## ファイル・ディレクトリ

| 種類 | パターン | 例 |
|------|----------|-----|
| ソース | `PascalCase.h/.cpp` | `World.h`, `JobSystem.cpp` |
| テスト | `〜Test.cpp` | `EcsTest.cpp` |
| ディレクトリ | `PascalCase/` | `Source/Engine/Ecs/` |

## 名前空間

- ルート: `NS::`
- モジュール: `NS::ECS::`, `NS::Graphics::`, `NS::Input::`
- サブ機能: `NS::ECS::Query::`
- 内部実装: `NS::ECS::Private::`

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
---
# モジュール構造

- 1モジュール = 1プロジェクト (.vcxproj)
- premake5.luaで各モジュールをproject()として定義

```
Source/Engine/
  ECS/                    ← ECS.vcxproj
    Public/               ← 他モジュールからインクルード可
    Private/              ← 実装 + 内部専用ヘッダ
  Graphics/               ← Graphics.vcxproj
    Public/
    Private/
```

## インクルード

- Public/は省略: `#include "ECS/World.h"`
- Private/内は同一モジュールのみ

## namespace

- Public/Private両方とも `NS::Module::`
- 内部専用型: `NS::Module::Private::`

---

# Plan Mode

## 使い分け

| 状況 | プラン形式 |
|------|-----------|
| 小さな変更、構造が明確 | コード付き |
| 大きな設計変更、方針比較 | 文章のみ |

## 計画に含めること

- 目的
- 変更対象ファイル
- アプローチ（概念レベル）
- 検証方法（ビルド/テスト/動作確認）
- 影響範囲
- TODOリスト（実装時にチェック）

```markdown
- [ ] ファイルA修正
- [ ] ファイルB追加
- [x] 完了したらチェック
```

※ TODO が 10 項目を超える場合 → 「大規模機能の計画」へ

## 文章のみプランの場合

コードは書かない。実装は承認後に改めて考える。

## 大きな変更時の確認

1. 既存実装を残すか削除するか質問
2. 削除する場合：新実装完成→動作確認→削除計画→削除実行

## 大規模機能の計画

### いつ使うか

通常の計画で TODO が 10 項目を超える、または複数の独立したサブシステムにまたがる場合。

### 構造

```
.claude/plans/<feature-name>/
  README.md           ← 概要とサブ計画へのリンク（TODO は書かない）
  <sub-topic-1>.md    ← 実装計画（TODO 必須）
  <sub-topic-2>.md
  ...
```

### 例: RHI実装

README.md にサブ計画へのリンクを列挙:
- [GPUデバイス抽象化](gpu-device.md)
- [テクスチャリソース](texture-resource.md)
- [コマンドバッファ](command-buffer.md)

各 .md は TODO 5 項目以内の実装計画。

### 細分化の基準

1 セッションで完了できるサイズまで分割する。
目安: 実装 TODO が 5 項目以内に収まればそれ以上分割しない。
超えるならさらにファイルを分ける。

### セッション再開時

新しいセッションや `/compact` 後は、まず `.claude/plans/` 配下の README.md を読み、
未完了のサブ計画から作業を再開すること。

### 計画の変更

実装中に計画と乖離した場合、先に計画ファイルを更新してから作業を続行する。

### TODO の更新

サブ計画の TODO は、実装完了時にその場でチェックを付けること。
全 TODO 完了後、README.md 側のリンクにも完了を明記する。

---

# コメント

- 日本語で書く

## エンジンヘッダー（パブリックAPI）

- `///` Doxygenコメントを書く
- 振る舞い・制約・null時の挙動など「ヘッダだけ見てわかる」情報を書く
- APIグループ分けのセクション区切り線は可

## クラス冒頭

- 概要と依存を書く（`/// 依存: X, Y`）

## エンジンcpp / ゲームコード

- 人間が書いたような簡潔なコメントにする
- 書く: なぜ、罠、パフォーマンス理由
- 書かない: 自明なコード

## 形式

- `TODO:` `HACK:` `FIXME:` は大文字+コロン

## 禁止

- `//!` `///<` 形式
- 「Phase 1」「Step 1」のような段階的説明
- コメントアウトしたコード（gitにある）
- セクション区切り線（エンジンヘッダーのAPIグループ分けは除く）
- コードの直訳（「ループする」「初期化する」等）
- 関数名をそのまま繰り返すだけのコメント
- 「以下のコードは〜」構文
- ブロック終端の `} // end of ...`
- 「Note:」「Important:」プレフィックス

---

# 開発ワークフロー

**Windowsコマンドは `powershell -Command` 経由で実行。シェルは `/usr/bin/bash`（Windows上でも）。パスは `/` で書く。**

```bash
# 1. パス指定
C:/Users/nanat/file.txt                    # ✓ フォワードスラッシュ
C:\Users\nanat\file.txt                    # ✗ エスケープと解釈される

# 2. Windowsコマンド実行
powershell -Command "dir"                  # ✓ powershell経由
dir                                        # ✗ bashで直接実行不可

# 3. hooks のパス
"bash \"C:/Users/nanat/Desktop/NS-ENGINE/.claude/hooks/hook.sh\""  # ✓ 絶対パス
"bash \"$PROJECT_ROOT/.claude/hooks/hook.sh\""                     # ✗ 環境変数は展開されない
```
