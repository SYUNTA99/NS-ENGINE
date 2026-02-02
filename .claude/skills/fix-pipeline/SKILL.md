---
description: "CI失敗の自動修正"
---

# Task
失敗したCIパイプラインを修正:

1. `gh run view --log-failed` で失敗ログを取得
2. 実際のエラーを分析（推測しない！）
3. 修正を実装
4. コミットしてプッシュ
