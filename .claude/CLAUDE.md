# NS-ENGINE

3D三人称視点プラットフォーマー/ジャンプアクションゲーム専用エンジン

## このファイルの方針

- 短く、具体的に、Claudeが自分で検証ループを回せる情報を書く
- 命名規則・禁止事項など「コードと照合できるルール」を書く
- アーキテクチャ詳細は書かない（コードを読めばわかる、陳腐化する）
- ビルド・テスト方法は書かない（スキルにある）

---
## ディレクトリ命名規則

- **フォルダ名: PascalCase**

# モジュール構造

- 1モジュール = 1プロジェクト (.vcxproj)
- premake5.luaで各モジュールをproject()として定義

## ディレクトリ構成（3層構造）

```
Source/Engine/
  [Module]/
    Public/
    Internal/
    Private/
```

**Public/** - Gameコードや外部モジュールが使う公開API
- 誰でもインクルード可能
- 例: `World.h`, `Entity.h`

**Internal/** - Engine内の他モジュールが使う内部共有型
- Gameコードからは使用不可
- 例: `Chunk.h`（ECS内部だがGraphicsも参照）

**Private/** - このモジュール内だけで使う実装
- 同一モジュールのみインクルード可能
- 実装ファイル(.cpp)、単一ファイル用ヘルパー

**配置判断:** Gameが使う→Public / Engine他モジュールが使う→Internal / 自モジュールだけ→Private

## インクルード

- Public/は省略: `#include "ECS/World.h"`
- Internal/も省略: `#include "ECS/Chunk.h"`（Engine内のみ）
- Private/内は同一モジュールのみ

## namespace

- Public/Internal/Private全て `NS::Module::`
- 内部専用型: `NS::Module::Private::`

---

# 開発ワークフロー

**Windowsコマンドは `powershell -Command` 経由で実行**

```bash
# 1. パス指定
C:/Users/nanat/file.txt                    # ✓ フォワードスラッシュ
C:\Users\nanat\file.txt                    # ✗ エスケープと解釈される

# 2. hooks のパス
"bash \"C:/Users/nanat/Desktop/NS-ENGINE/.claude/hooks/hook.sh\""  # ✓ 絶対パス
"bash \"$PROJECT_ROOT/.claude/hooks/hook.sh\""                     # ✗ 環境変数は展開されない
```
