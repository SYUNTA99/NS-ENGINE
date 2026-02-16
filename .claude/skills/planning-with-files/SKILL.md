---
name: planning-with-files
version: "2.10.0-custom"
description: Implements Manus-style file-based planning for complex tasks. Creates task_plan.md, findings.md, and progress.md in .claude/plans/current/. OpenSpec双方向連携: 開始時にopenspec/specs/を読み込み、完了時に確定仕様を反映。Use when starting complex multi-step tasks, research projects, or any task requiring >5 tool calls.
user-invocable: true
allowed-tools:
  - Read
  - Write
  - Edit
  - Bash
  - Glob
  - Grep
  - WebFetch
  - WebSearch
hooks:
  PreToolUse:
    - matcher: "Write|Edit|Bash|Read|Glob|Grep"
      hooks:
        - type: command
          # ツール実行前に現在の計画を表示
          command: "cat .claude/plans/current/task_plan.md 2>/dev/null | head -30 || true"
  PostToolUse:
    - matcher: "Write|Edit"
      hooks:
        - type: command
          # ファイル更新後のリマインダー
          command: "echo '[planning-with-files] File updated. If this completes a phase, update task_plan.md status.'"
---

# Planning with Files

## 2層計画システム

| レイヤー | 目的 | スキル | 配置場所 |
|----------|------|--------|----------|
| **計画（Plan）** | 大規模機能の分割・追跡 | `/plan-large-scale` | `.claude/plans/<feature>/` |
| **実装（Work）** | 作業記憶の永続化 | このスキル | `.claude/plans/current/` |

計画レイヤーは `/plan-large-scale` を参照。

## Mode（調査 / 実装）

current/ は2つのモードで使用される:

| Mode | 用途 | task_plan.md の記載 |
|------|------|---------------------|
| `investigation` | 大規模機能の事前調査 | `Mode: investigation` |
| `implementation` | 通常のタスク実行 | `Mode: implementation` |

- 調査モードは `/plan-large-scale` の Phase 0 専用。テンプレート: [templates/investigation_plan.md](templates/investigation_plan.md)
- 実装モードは単独タスクでも、サブ計画の実装でも使用可能
- サブ計画を実装する場合は「実装中: `<親計画リンク>`」を追記

---

Work like Manus: Use persistent markdown files as your "working memory on disk."

<!-- このプロジェクトでは .claude/plans/ 配下に配置 -->

## Important: Where Files Go

```
.claude/plans/
├── current/              # 現在進行中の計画
│   ├── task_plan.md      # フェーズ・進捗
│   ├── findings.md       # 調査結果・発見
│   └── progress.md       # セッションログ
└── <feature-name>/       # 完了した計画のアーカイブ
```

| Location | What Goes There |
|----------|-----------------|
| `.claude/plans/current/` | Active planning files |
| `.claude/plans/<feature>/` | Completed/archived plans |

## OpenSpec 連携

タスクが **新機能の設計・API追加・仕様変更** を含む場合のみ連携する。
バグ修正・リファクタリング・ドキュメント更新など仕様に影響しないタスクでは一切スキップ。

### 開始時: コンテキスト読み込み

タスク内容が仕様関連と判断した場合のみ:

```bash
powershell -Command "& '.claude/skills/planning-with-files/scripts/openspec-context.ps1' -Module '<module>'"
```

関連 spec があれば読み、`findings.md` の「OpenSpec コンテキスト」に要約を記載。

### 完了時: 仕様フィードバック

実装で **新しいPublic API・動作仕様の変更・既存仕様の修正** が確定した場合のみ:
ユーザーに「OpenSpec に仕様を反映しますか？」と確認。承認時 `/opsx:sync` で反映。

## Quick Start

Before ANY complex task:

1. **Create `.claude/plans/current/` directory**
2. **Create `task_plan.md`** — Use [templates/task_plan.md](templates/task_plan.md) as reference
3. **Create `findings.md`** — Use [templates/findings.md](templates/findings.md) as reference（OpenSpec コンテキストを含む）
4. **Create `progress.md`** — Use [templates/progress.md](templates/progress.md) as reference
5. **Re-read plan before decisions** — Refreshes goals in attention window
6. **Update after each phase** — Mark complete, log errors

## The Core Pattern

```
Context Window = RAM (volatile, limited)
Filesystem = Disk (persistent, unlimited)

→ Anything important gets written to disk.
```

## File Purposes

| File | Purpose | When to Update |
|------|---------|----------------|
| `task_plan.md` | Phases, progress, decisions | After each phase |
| `findings.md` | Research, discoveries | After ANY discovery |
| `progress.md` | Session log, test results | Throughout session |

## Critical Rules

### 1. Create Plan First
Never start a complex task without `task_plan.md`. Non-negotiable.

### 2. The 2-Action Rule
> "After every 2 view/browser/search operations, IMMEDIATELY save key findings to text files."

This prevents visual/multimodal information from being lost.

### 3. Read Before Decide
Before major decisions, read the plan file. This keeps goals in your attention window.

### 4. Update After Act
After completing any phase:
- Mark phase status: `in_progress` → `complete`
- Log any errors encountered
- Note files created/modified

### 5. Log ALL Errors
Every error goes in the plan file. This builds knowledge and prevents repetition.

```markdown
## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| FileNotFoundError | 1 | Created default config |
| API timeout | 2 | Added retry logic |
```

### 6. Never Repeat Failures
```
if action_failed:
    next_action != same_action
```
Track what you tried. Mutate the approach.

## The 3-Strike Error Protocol

```
ATTEMPT 1: Diagnose & Fix
  → Read error carefully
  → Identify root cause
  → Apply targeted fix

ATTEMPT 2: Alternative Approach
  → Same error? Try different method
  → Different tool? Different library?
  → NEVER repeat exact same failing action

ATTEMPT 3: Broader Rethink
  → Question assumptions
  → Search for solutions
  → Consider updating the plan

AFTER 3 FAILURES: Escalate to User
  → Explain what you tried
  → Share the specific error
  → Ask for guidance
```

## Read vs Write Decision Matrix

| Situation | Action | Reason |
|-----------|--------|--------|
| Just wrote a file | DON'T read | Content still in context |
| Viewed image/PDF | Write findings NOW | Multimodal → text before lost |
| Browser returned data | Write to file | Screenshots don't persist |
| Starting new phase | Read plan/findings | Re-orient if context stale |
| Error occurred | Read relevant file | Need current state to fix |
| Resuming after gap | Read all planning files | Recover state |

## The 5-Question Reboot Test

If you can answer these, your context management is solid:

| Question | Answer Source |
|----------|---------------|
| Where am I? | Current phase in task_plan.md |
| Where am I going? | Remaining phases |
| What's the goal? | Goal statement in plan |
| What have I learned? | findings.md |
| What have I done? | progress.md |

## When to Use This Pattern

**Use for:**
- Multi-step tasks (3+ steps)
- Research tasks
- Building/creating projects
- Tasks spanning many tool calls
- Anything requiring organization

**Skip for:**
- Simple questions
- Single-file edits
- Quick lookups

## On Task Completion

タスク完了時、**ユーザーの明示的な指示があるまで current/ を初期化・削除しない**。

完了時チェックリスト:
1. 実装で新しい仕様が確定した場合 → OpenSpec 仕様フィードバックを提案（上記「完了時」参照）
2. ユーザーがアーカイブを指示した場合のみ:

```bash
mv .claude/plans/current .claude/plans/<feature-name>
```

## Anti-Patterns

| Don't | Do Instead |
|-------|------------|
| Use TodoWrite for persistence | Create task_plan.md file |
| State goals once and forget | Re-read plan before decisions |
| Hide errors and retry silently | Log errors to plan file |
| Stuff everything in context | Store large content in files |
| Start executing immediately | Create plan file FIRST |
| Repeat failed actions | Track attempts, mutate approach |
