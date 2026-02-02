📋 /prd-generator - PRD自動生成
背景と種類
PRD生成系のコマンド/スキルは複数のバリエーションが存在します。
名前作者形式特徴/prd-generatorDenis Redozubovプラグイン会話コンテキストからPRD生成prd-generatorjamesrochabrunスキル対話的ヒアリング→PRD生成prd-taskmasteranombyte93スキルTaskmaster連携、タスク分解まで/create-prdbuildwithclaudeコマンド機能説明からPRD生成/create-plantaddyorgコマンド標準化フォーマットのPRD

/prd-generator (Denis Redozubov)
Denis Redozubovの/prd-generatorはClaude Codeプラグインで、会話コンテキストからPRDを生成します。要件について議論した後に/create-prdを呼び出すと、Executive Summary、User Stories、MVPスコープ、アーキテクチャ、成功基準、実装フェーズなど全セクションを含む完全なPRDを生成します。 GitHub
基本的な使い方
# Step 1: まず要件について自由に会話する
> 「ユーザー認証機能を作りたい。OAuthとメール認証の両方をサポートして、
    2FAもオプションで付けたい」

# Step 2: 十分に議論した後にコマンドを呼び出す
> /create-prd
PRDに含まれるセクション
markdown# Product Requirements Document

## 1. Executive Summary（概要）
- 2-3段落のハイレベル概要

## 2. Problem Statement（課題定義）
- 何を解決するのか

## 3. User Stories（ユーザーストーリー）
- As a [ユーザータイプ], I want to [行動], So that [価値]
- 各ストーリーにAcceptance Criteria付き

## 4. MVP Scope（最小実行可能スコープ）
- In Scope / Out of Scope の明確な線引き

## 5. Architecture（アーキテクチャ）
- 技術的なアプローチ、依存関係

## 6. Success Criteria（成功基準）
- KPI、測定方法

## 7. Implementation Phases（実装フェーズ）
- フェーズ分けされたロードマップ

prd-generator スキル（jamesrochabrun版）の詳細
より体系的なスキル実装で、対話的なヒアリングプロセスを経てPRDを生成します。
SKILL.md の構造
markdown---
name: prd-generator
description: Generate comprehensive Product Requirements Documents (PRDs)
  for product managers. Use this skill when users ask to "create a PRD",
  "write product requirements", "document a feature", or need help
  structuring product specifications.
---
ワークフロー（5ステップ）
Step 1: コンテキスト収集（Discovery）
markdown## 必須ヒアリング項目
1. 何を解決しようとしているか？
2. 主なユーザー/対象者は誰か？
3. 主要なビジネス目標は？
4. 技術的な制約はあるか？
5. 成功とはどのような状態か？どう測定するか？
6. タイムラインは？
7. 明示的にスコープ外なものは？
```

**ポイント**: ユーザーが詳細なブリーフを最初に提供した場合は質問をスキップし、不足している重要情報のみ確認する。

#### Step 2: PRD構造生成（13セクション）
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
Step 3: ユーザーストーリー作成
markdownAs a [user type],
I want to [action],
So that [benefit/value].

Acceptance Criteria:
- [具体的、テスト可能な基準 1]
- [具体的、テスト可能な基準 2]
- [具体的、テスト可能な基準 3]
```

#### Step 4: 成功指標の定義

製品タイプに応じたフレームワークを選択：
```
AARRR (Pirate Metrics):  Acquisition → Activation → Retention → Revenue → Referral
HEART Framework:          Happiness → Engagement → Adoption → Retention → Task Success
North Star Metric:        コアバリューを表す単一の主要指標
OKRs:                     Objectives and Key Results
Step 5: 検証＆レビュー
bash# バリデーションスクリプトで完全性チェック
scripts/validate_prd.sh my_prd.md

# チェック項目:
# - 全必須セクションが存在するか
# - ユーザーストーリーが正しいフォーマットか
# - 成功指標が定義されているか
# - スコープが明確か
# - プレースホルダーテキストが残っていないか
```

### PRDテンプレートの種類
```
Standard PRD    → 包括的な完全版ドキュメント
Lean PRD        → アジャイルチーム向けの簡潔版
One-Pager       → エグゼクティブサマリー形式
Technical PRD   → エンジニアリング重視の要件定義
Design PRD      → UX/UI重視の要件定義
```

使い分け例：
```
「Lean PRDを作って...」  → 簡潔版
「Technical PRDを...」   → エンジニア向け詳細版
実用パターン例
markdown## パターン1: 新機能PRD
User: 「モバイルアプリにダークモードを追加するPRDを作って」
→ テーマ切替、設定永続化、システム連動、デザイントークン更新のストーリー生成
→ 採用率・ユーザー満足度をメトリクスに設定

## パターン2: 製品改善PRD
User: 「検索機能の改善要件を書いて」
→ 現状分析 → 改善提案 → 影響評価 → Before/Afterメトリクス

## パターン3: 新製品PRD
User: 「新しいアナリティクスダッシュボード製品のPRDが必要」
→ 市場機会 → 競合分析 → 製品ビジョン → MVPスコープ → GTM考慮事項

## パターン4: クイックPRD / ワンページャー
User: 「小さなバグ修正のための軽量PRDを作って」
→ 課題定義 → 解決アプローチ → 受け入れ基準 → 成功指標（1-2ページ）

prd-taskmaster（Taskmaster連携版）
prd-taskmasterはClaude Codeスキルで、Taskmasterなどのタスク分解ツールとシームレスに連携するよう設計された、エンジニア向けのPRDを生成します。 GitHub
bash# インストール
cd ~/.claude/skills
git clone https://github.com/anombyte93/prd-taskmaster.git

# 使い方
claude
> 「ダークモード追加のPRDが欲しい」
# → スキルが自動認識・起動
```

### 出力例
```
📄 PRD Created: .taskmaster/docs/prd.md
🤖 CLAUDE.md Generated: TDDワークフローガイド

📊 Overview:
- Feature: Two-Factor Authentication
- Complexity: Medium
- Estimated Effort: 26 tasks, ~119 hours

🎯 Key Requirements:
1. REQ-001: TOTP/SMS 2FA support
2. REQ-002: Backup codes for recovery
3. REQ-003: Login flow integration

⚠️ Quality Validation: 58/60 (EXCELLENT ✅)

🚀 Next Steps:
1. Review PRD: .taskmaster/docs/prd.md
2. taskmaster init
3. taskmaster parse-prd --input .taskmaster/docs/prd.md
4. taskmaster next-task  # 実装開始
```

### ディレクトリ構成
```
.taskmaster/
├── docs/
│   ├── prd.md              # 生成されたPRD
│   └── architecture.md     # アーキテクチャドキュメント
├── tasks/
│   └── .gitkeep
└── notes/
    └── .gitkeep

🎓 /explain-code - コードをアナロジーとASCII図で説明
公式ドキュメントの実装例
explain-codeはClaude Code公式ドキュメントで紹介されているスキルの例です。~/.claude/skills/explain-code/SKILL.mdに配置し、コードの仕組みを説明する際やユーザーが「これはどう動くの？」と質問した時に自動的に使用されます。 Claude
SKILL.md
markdown---
name: explain-code
description: Explains code with visual diagrams and analogies.
  Use when explaining how code works, teaching about a codebase,
  or when the user asks "how does this work?"
---

When explaining code, always include:

1. **Start with an analogy**: Compare the code to something
   from everyday life

2. **Draw a diagram**: Use ASCII art to show the flow,
   structure, or relationships

3. **Walk through the code**: Explain step-by-step what happens

4. **Highlight a gotcha**: What's a common mistake
   or misconception?
```

### 配置場所
```
~/.claude/skills/
└── explain-code/
    └── SKILL.md
```

### スキル vs スラッシュコマンドとしての使い分け

スキルは`description`フィールドに基づいてClaudeがコンテキストに応じて自動的に起動する仕組みです。一方スラッシュコマンドは明示的に`/command-name`で呼び出します。 
```
# スキルとして（自動起動）
> 「このコードはどう動くの？」
→ Claudeがdescriptionを見て自動でexplain-codeスキルを適用

# スラッシュコマンドとして（明示呼び出し）
> /explain-code src/auth/middleware.js
→ 手動でトリガー
```

---

## 出力イメージ：具体例

### 例1: Express ミドルウェアの説明
```
> explain this middleware

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🎯 ANALOGY: Airport Security Checkpoint

This middleware works like airport security.
Every passenger (request) must pass through
the checkpoint (middleware) before reaching
their gate (route handler). The guard checks
your boarding pass (JWT token) — if it's valid,
you proceed; if not, you're turned away.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📐 DIAGRAM:

  Request
    │
    ▼
┌─────────────────┐
│  authMiddleware  │
│                  │
│  ① Extract token │
│     from header  │
│         │        │
│         ▼        │
│  ② Verify JWT    │
│    ┌────┴────┐   │
│  Valid?    Invalid│
│    │         │   │
│    ▼         ▼   │
│  next()   401    │
│            Error  │
└────┬────────┬────┘
     │        │
     ▼        ▼
  Route    Client
  Handler  (rejected)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📖 WALKTHROUGH:

Line 1-3: Function receives (req, res, next)
  → The "security guard" gets the passenger info

Line 5: const token = req.headers.authorization?.split(' ')[1]
  → Extracts the "boarding pass" from the
    "Authorization: Bearer xxx" header

Line 7-9: jwt.verify(token, SECRET_KEY)
  → Checks if the boarding pass is genuine
  → If valid, attaches decoded user to req.user

Line 11: next()
  → Waves the passenger through to their gate

Line 13-15: catch block → res.status(401)
  → Invalid pass → "Access Denied"

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

⚠️ GOTCHA:

Don't forget to handle expired tokens separately!
jwt.verify throws a TokenExpiredError, which you
might want to handle differently from a malformed
token — returning "Please log in again" instead
of "Access denied".
```

### 例2: React useState の説明
```
> how does useState work?

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🎯 ANALOGY: Whiteboard with a Camera

Imagine a whiteboard in a meeting room.
useState is like writing a value on the
whiteboard (state) and having a camera
that takes a photo every time you erase
and rewrite (re-render). The "setter"
is your eraser + marker combo.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📐 DIAGRAM:

  const [count, setCount] = useState(0)
         │         │              │
         │         │              └─ Initial value
         │         │                 (written on whiteboard)
         │         │
         │         └─ Setter function
         │            (eraser + marker)
         │
         └─ Current value
            (what's on the whiteboard NOW)


  ┌──────────┐  setCount(1)  ┌──────────┐
  │ count: 0 │ ───────────▶  │ count: 1 │
  │          │   (erase &    │          │
  │ Render 1 │    rewrite)   │ Render 2 │
  └──────────┘               └──────────┘
       📸                         📸
   (snapshot)                 (snapshot)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

⚠️ GOTCHA:

State updates are ASYNCHRONOUS!

  setCount(count + 1)
  setCount(count + 1)  // ← Still uses OLD count!
  // Result: count is 1, not 2

Fix: Use the functional form:
  setCount(prev => prev + 1)
  setCount(prev => prev + 1)  // ← Uses latest!
  // Result: count is 2 ✓
```

### 例3: データベースのインデックス
```
> explain database indexes

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🎯 ANALOGY: Library Card Catalog

A database index is like the card catalog
in a library. Without it, finding a book
means walking every shelf (full table scan).
With a card catalog (index), you look up
"Shakespeare" → "Aisle 7, Shelf 3" and
go directly there.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📐 DIAGRAM:

WITHOUT INDEX (Full Table Scan):

  SELECT * FROM users WHERE email = 'bob@...'

  ┌─────┬─────────────┬─────┐
  │ id  │ email       │ ... │
  ├─────┼─────────────┼─────┤
  │  1  │ alice@...   │     │  ← scan
  │  2  │ charlie@... │     │  ← scan
  │  3  │ bob@...     │     │  ← FOUND! (but keeps going)
  │  4  │ diana@...   │     │  ← scan
  │ ... │ ...         │     │  ← scan ALL rows
  └─────┴─────────────┴─────┘
  Time: O(n) 🐢


WITH INDEX (B-Tree Lookup):

  Index on "email":
              ┌───────┐
              │  D    │
              ├───┬───┤
             ╱         ╲
      ┌─────┐           ┌─────┐
      │ A-C │           │ E-Z │
      └──┬──┘           └─────┘
         │
    ┌────┴────┐
    │ alice   │ → row 1
    │ bob     │ → row 3  ← FOUND! (stop)
    │ charlie │ → row 2
    └─────────┘
  Time: O(log n) 🚀

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

⚠️ GOTCHA:

Indexes speed up reads but SLOW DOWN writes!
Every INSERT/UPDATE must also update the index.
Don't index every column — only those you
frequently WHERE, JOIN, or ORDER BY on.

カスタム拡張版の例
公式の4ルールを拡張して、より充実したexplain-codeスキルを作ることもできます：
markdown---
name: explain-code
description: Explains code with visual diagrams, analogies, 
  and progressive detail levels. Use when explaining how code 
  works, teaching about a codebase, or when the user asks 
  "how does this work?"
---

When explaining code, ALWAYS follow this structure:

## 1. One-Sentence Summary
Start with a single sentence: "This code does X."

## 2. Analogy from Everyday Life
Compare to something non-technical:
- Data structures → physical objects (stack = plate stack)
- Algorithms → real-world processes (sorting = organizing cards)
- Patterns → social systems (observer = newsletter subscription)
- Async → real-world concurrency (restaurant kitchen)

## 3. ASCII Diagram
Choose the best visualization:
- **Flow chart**: for sequential processes
- **Tree diagram**: for hierarchies / recursion
- **Sequence diagram**: for multi-component communication
- **State diagram**: for state machines
- **Box diagram**: for architecture / relationships

Use these symbols consistently:
  ─── │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼  (box drawing)
  ──▶ ──▷  (arrows)
  ✓ ✗ ⚠  (status)
  ① ② ③  (numbered steps)

## 4. Step-by-Step Walkthrough
Reference specific line numbers. Explain:
- WHAT each section does
- WHY it's done that way
- HOW data transforms at each step

## 5. Gotcha / Common Mistake
Highlight ONE thing that trips people up.
Show the wrong way AND the right way.

## 6. Related Concepts (optional)
If helpful, mention 1-2 related topics
the reader might want to explore next.

まとめ：2つのコマンドの位置づけ
/prd-generator/explain-code種類プラグイン / スキルスキル（公式サンプル）トリガー/create-prd で明示呼び出し「これどう動くの？」で自動起動入力会話コンテキスト / 引数コードファイル / 関数出力構造化されたMarkdown PRDアナロジー + ASCII図 + 解説対象者PM / プロダクトオーナー開発者 / 学習者複雑度高（13セクション、検証付き）中（4-6ステップ構造）