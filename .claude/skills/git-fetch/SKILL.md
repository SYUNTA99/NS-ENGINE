---
description: "リモートの最新情報を取得"
model: haiku
allowed-tools:
  - "Bash(git fetch:*)"
---

以下のコマンドを実行してください:

```bash
git fetch --all --prune
```

これにより:
- すべてのリモートから最新情報を取得
- 削除されたリモートブランチの参照を削除
