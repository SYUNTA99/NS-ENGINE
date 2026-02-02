---
description: "現在のブランチをpush"
model: haiku
allowed-tools:
  - "Bash(git push:*)"
  - "Bash(git branch:*)"
---

現在のブランチをリモートにpushしてください。

**実行コマンド**（Bashツールでそのまま実行）:
```bash
git push -u origin $(git branch --show-current)
```

これにより上流が未設定でも自動的に設定されます。
