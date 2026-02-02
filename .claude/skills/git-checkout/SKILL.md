---
description: "ブランチを切り替え"
model: haiku
allowed-tools:
  - "Bash(git checkout:*)"
  - "Bash(git branch:*)"
---

指定されたブランチにチェックアウト:

```bash
git checkout <branch-name>
```

引数がない場合は、ブランチ一覧を表示してユーザーに選択を促してください:
```bash
git branch -a
```
