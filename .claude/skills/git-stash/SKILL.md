---
description: "git stash 操作"
model: haiku
allowed-tools:
  - "Bash(git stash:*)"
  - "Bash(git status:*)"
---

引数に基づいてstash操作を実行してください:

## `/stash` または `/stash save` - 変更を一時保存
```bash
git stash push -m "WIP: 作業中の変更"
```

## `/stash l` または `/stash list` - stash一覧
```bash
git stash list
```

## `/stash p` または `/stash pop` - 最新のstashを適用して削除
```bash
git stash pop
```

## `/stash a` または `/stash apply` - 最新のstashを適用（削除しない）
```bash
git stash apply
```

## `/stash d` または `/stash drop` - 最新のstashを削除
```bash
git stash drop
```
