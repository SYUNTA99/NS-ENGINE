---
description: "要件定義書（PRD）を自動生成。「PRDを作って」「要件定義書が欲しい」「機能仕様を書いて」などで自動起動"
---

# PRD Generator

プロジェクトや機能の要件定義書（Product Requirements Document）を対話的に生成します。

## 使用方法
`/prd-generator $ARGUMENTS`

`$ARGUMENTS` に機能名や概要を指定してください。
引数がない場合は、ユーザーに対象を確認してください。

または会話で要件について議論した後に `/prd-generator` を呼び出す。

## ワークフロー

### Step 1: コンテキスト収集（Discovery）

以下を確認（ユーザーが既に詳細を提供している場合はスキップ）：

1. 何を解決しようとしているか？
2. 主なユーザー/対象者は誰か？
3. 主要なビジネス目標は？
4. 技術的な制約はあるか？
5. 成功とはどのような状態か？どう測定するか？
6. タイムラインは？
7. 明示的にスコープ外なものは？

### Step 2: PRD構造生成（13セクション）

```
1.  Executive Summary      - ハイレベル概要（2-3段落）
2.  Problem Statement      - 課題の明確な表現
3.  Goals & Objectives     - 達成目標
4.  User Personas          - ターゲットユーザー像
5.  User Stories           - 機能要件の詳細
6.  Success Metrics        - KPIと測定基準
7.  Scope                  - In/Out of Scope
8.  Technical Considerations - アーキテクチャ、依存関係、制約
9.  Design & UX Requirements - UI/UXの考慮事項
10. Timeline & Milestones  - 主要日程とフェーズ
11. Risks & Mitigation     - リスクと対策
12. Dependencies & Assumptions - 依存関係と前提条件
13. Open Questions         - 未解決事項
```

### Step 3: ユーザーストーリー形式

```
As a [user type],
I want to [action],
So that [benefit/value].

Acceptance Criteria:
- [具体的、テスト可能な基準 1]
- [具体的、テスト可能な基準 2]
- [具体的、テスト可能な基準 3]
```

### Step 4: 成功指標フレームワーク

製品タイプに応じて選択：
- **AARRR (Pirate Metrics)**: Acquisition → Activation → Retention → Revenue → Referral
- **HEART Framework**: Happiness → Engagement → Adoption → Retention → Task Success
- **North Star Metric**: コアバリューを表す単一の主要指標
- **OKRs**: Objectives and Key Results

### Step 5: バリデーション

生成後、以下をチェック：
- 全必須セクションが存在するか
- ユーザーストーリーが正しいフォーマットか
- 成功指標が定義されているか
- スコープが明確か
- プレースホルダーテキストが残っていないか

## テンプレート種類

ユーザーの要望に応じて使い分け：

| 種類 | 用途 |
|------|------|
| Standard PRD | 包括的な完全版ドキュメント |
| Lean PRD | アジャイルチーム向けの簡潔版 |
| One-Pager | エグゼクティブサマリー形式 |
| Technical PRD | エンジニアリング重視の要件定義 |
| Design PRD | UX/UI重視の要件定義 |

## 使い分け例

```
「Lean PRDを作って...」  → 簡潔版
「Technical PRDを...」   → エンジニア向け詳細版
「1ページでまとめて」    → One-Pager
```
