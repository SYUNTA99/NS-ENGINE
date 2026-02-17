---
name: plan-large-scale
description: 大規模機能の分割と追跡。以下のいずれかに該当する場合に使用: (1) 新規ファイル作成が5つ以上 (2) 変更モジュールが2つ以上 (3) 既存APIの破壊的変更 (4) 複数サブシステムの協調動作が必要。計画レイヤーとして .claude/plans/<feature>/ に永続的な計画ファイルを作成し、実装レイヤー（/planning-with-files）と連携する。
---

# Plan Large Scale

大規模機能を分割し、追跡するための計画スキル。

## トリガー条件

以下のいずれかに該当する場合、このスキルを使用:

| 条件 | 閾値 |
|------|------|
| 新規ファイル作成 | ≥ 5 |
| 変更モジュール数 | ≥ 2 |
| 既存APIの破壊的変更 | あり |
| 複数サブシステムの協調 | 必要 |
| 依存関係の追加/削除 | あり |

該当しない場合は `/planning-with-files` のみで十分。

## 2層計画システム

| レイヤー | 目的 | スキル | 配置場所 |
|----------|------|--------|----------|
| **計画** | 機能の分割・追跡 | このスキル | `.claude/plans/<feature>/` |
| **実装** | 作業記憶の永続化 | `/planning-with-files` | `.claude/plans/current/` |

## ワークフロー概要

### Phase 0: 調査（/planning-with-files 調査モード）

大規模機能に着手する前に、まず調査を行う:

```
1. /planning-with-files で current/ を調査用に初期化
2. コードベースを探索
   - 関連ファイル・モジュールを特定
   - 既存パターンを把握
   - 依存関係を整理
   - 必要なAPI/機能を洗い出し
3. findings.md に調査結果を蓄積
4. 調査完了後、ユーザーに報告
```

### Phase 1: 計画作成（/plan-large-scale）

調査結果を基に計画を作成:

```
1. findings.md を精査し、機能を分析
   - 全体の目的を明確化
   - 必要なサブシステムを洗い出し
   - 依存関係・実行順序を決定
2. サブ計画に分割
   - 各5 TODO以内（上限基準）
   - 最低サブ計画数を規模表から算出し、下回らないこと
   - 必須分割ルールを適用（モジュール境界・Public API・破壊的変更）
3. .claude/plans/<feature>/ に計画ファイル作成
4. current/findings.md を <feature>/00-investigation.md として保存
5. ユーザー承認待ち
```

### Phase 2-N: 実装（/planning-with-files 実装モード）

```
1. /planning-with-files で current/ を実装用に初期化
2. task_plan.md に「実装中: <feature>/01-xxx.md」を記録
3. サブ計画の TODO を実装
4. 完了後、知見を親計画に転記 → 次のサブ計画へ
```

詳細フロー図: [references/workflow.md](references/workflow.md)

## 計画ファイル構造

```
.claude/plans/<feature-name>/
├── README.md           # 概要 + サブ計画リンク（TODO書かない）
├── 01-<sub-topic>.md   # サブ計画（TODO 5項目以内）
├── 02-<sub-topic>.md
└── ...
```

## README.md テンプレート

```markdown
# <Feature Name>

## 目的

<!-- 何を実現するか -->

## 影響範囲

<!-- どのモジュール・ファイルに影響するか -->

## サブ計画

| # | 計画 | 状態 |
|---|------|------|
| 1 | [サブ計画1](01-xxx.md) | pending |
| 2 | [サブ計画2](02-xxx.md) | pending |
| 3 | [サブ計画3](03-xxx.md) | pending |

## 検証方法

<!-- 全体が完了したときの検証方法 -->

## 設計決定

<!-- 重要な設計判断とその理由 -->
```

## サブ計画の書き方

**読者はAI（Claude）。人間向けドキュメントではない。**

短く、具体的に、Claudeが自分で検証ループを回せる情報だけ書く。

### 含めるもの

- **対象ファイル**: フルパス (`Source/Engine/RHI/Public/RHIFwd.h`)
- **TODO**: 動詞+対象+場所 (`IRHIBuffer の Map/Unmap を実装 → RHIStagingBuffer.cpp`)
- **依存**: サブ計画番号+シンボル名 (`depends: 03-02 (IRHIBuffer)`)
- **API設計**: シグネチャのみ。実装コードは書かない
- **検証**: 実行可能なコマンドまたは確認手順 (`build --module RHI` / `grep -r "IRHIBuffer" Source/`)

### 書かないもの

- 実装コード全文（AIが生成する）
- 使用例（検証に必要なら検証セクションに書く）
- 概念説明・背景（AIは知っている）
- プラットフォーム比較表（実装時に調べる）

## 細分化の基準

### サブ計画の上限（1つあたり）

- 新規ファイル ≤ 3
- 変更ファイル ≤ 5
- TODO ≤ 5

超えるならさらに分割。

### 最低サブ計画数（規模から算出）

以下の表から該当する最大値を最低数とする:

| 指標 | 規模 | 最低サブ計画数 |
|------|------|---------------|
| 新規ファイル | 1-2 | 1 |
| 新規ファイル | 3-4 | 2 |
| 新規ファイル | 5-9 | 3 |
| 新規ファイル | 10-19 | 5 |
| 新規ファイル | 20+ | 8 |
| 変更モジュール | 2-3 | モジュール数と同数 |
| 変更モジュール | 4+ | モジュール数 + 1 |
| 破壊的変更 | あり | +1（隔離用） |

例: 新規12ファイル・3モジュール・破壊的変更あり → max(5, 3, 3+1) = **最低5サブ計画**

### 必須分割ルール

- **モジュール境界**: 異なるモジュールの変更は別サブ計画にする
- **Public API vs 実装**: Public/ の新規・変更は内部実装と分離する
- **破壊的変更の隔離**: 既存APIの破壊的変更は専用サブ計画に切り出す
- **テスト・検証**: 統合テストや全体検証は最後のサブ計画にまとめる

## current/ のルール

- **固定配置**: 実装ファイルは常に `current/` に配置
- **1つのみ**: 並行作業禁止
- **上書き**: 次のサブ計画開始時に初期化
- **知見転記**: `findings.md` → 親サブ計画.md に転記してから次へ

## セッション再開時

`.claude/plans/` を確認:
- `current/` あり → `task_plan.md` の「実装中」リンクから再開
- `current/` なし → 計画フォルダの README.md から未完了を確認

## スクリプト

| スクリプト | 用途 |
|-----------|------|
| `scripts/init-investigation.ps1` | 調査セッション初期化 |
| `scripts/archive-investigation.ps1` | 調査結果をアーカイブ |
| `scripts/init-feature.ps1` | 計画ディレクトリ初期化 |
| `scripts/feature-status.ps1` | 進捗確認 |
| `scripts/next-subplan.ps1` | 次のサブ計画を特定 |

### init-investigation.ps1

調査セッションを初期化し、current/ に調査用テンプレートを配置する。

```powershell
.\init-investigation.ps1 <feature-name>

# 例: RHI の調査を開始
.\init-investigation.ps1 rhi
```

### archive-investigation.ps1

調査完了後、findings.md を計画ディレクトリに 00-investigation.md として保存する。

```powershell
.\archive-investigation.ps1 <feature-name>

# 例: RHI の調査結果をアーカイブ
.\archive-investigation.ps1 rhi
```

### init-feature.ps1

計画ディレクトリを作成し、README.md テンプレートを配置する。

```powershell
.\init-feature.ps1 <feature-name> [subplan-count]

# 例: 計画ディレクトリのみ作成
.\init-feature.ps1 rhi

# 例: サブ計画ファイル3つを同時に作成
.\init-feature.ps1 rhi 3
```

### feature-status.ps1

機能全体の進捗を表示する。

```powershell
.\feature-status.ps1 <feature-name>

# 出力例:
# === Feature Status: rhi ===
#
# Total subplans:  3
# Complete:        1
# Pending:         2
# Progress:        33%
#
# [X] 01-gpu-device.md
# [ ] 02-texture-resource.md
# [ ] 03-command-buffer.md
```

### next-subplan.ps1

次に着手すべきサブ計画を特定する。

```powershell
.\next-subplan.ps1 <feature-name>

# 出力例:
# Next subplan: .claude/plans/rhi/02-texture-resource.md
```
