---
description: "git log を簡潔に表示"
model: haiku
allowed-tools:
  - "Bash(git log:*)"
---

`$ARGUMENTS` が指定された場合は、その件数分だけ表示します。
指定がない場合はデフォルト20件を表示します。

**実行コマンド**:
```bash
git log --oneline -$ARGUMENTS
```

デフォルト（引数なし）:
```bash
git log --oneline -20
```
