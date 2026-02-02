---
description: "PR作成ワークフロー（ブランチ作成→コミット→PR提出）"
---

# Context
- Current branch: !`git branch --show-current`
- Git status: !`git status`
- Recent commits: !`git log --oneline -10`

# Task
PR作成をガイド:
1. テストとリントを実行
2. git diffで意図しない変更がないか確認
3. 関連する変更をステージ
4. Conventional Commitメッセージを作成
5. PRサマリーを生成:
   - 何を変更したか、なぜか
   - 実行したテスト
   - 破壊的変更（あれば）
6. `gh pr create` でPRを作成
