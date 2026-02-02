---
description: "git diff を表示"
model: haiku
allowed-tools:
  - "Bash(git diff:*)"
  - "Bash(git status:*)"
---

引数に基づいて差分を表示してください:

## `/diff` - ステージされていない変更
```bash
git diff
```

## `/diff staged` または `/diff s` - ステージ済みの変更
```bash
git diff --staged
```

## `/diff main` - mainブランチとの差分
```bash
git diff main...HEAD
```

## `/diff <commit>` - 特定コミットとの差分
```bash
git diff <commit>
```
