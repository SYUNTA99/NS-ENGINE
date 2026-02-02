---
description: "ブランチ操作 (l=一覧, c=作成, d=削除)"
model: haiku
allowed-tools:
  - "Bash(git branch:*)"
  - "Bash(git checkout:*)"
---

引数に基づいて以下のコマンドを実行してください:

## `/branch` または `/branch l` - ブランチ一覧
```bash
git branch -a
```

## `/branch c <name>` - 新規ブランチ作成
```bash
git checkout -b <name>
```

## `/branch d <name>` - ブランチ削除
```bash
git branch -d <name>
```

削除に失敗した場合（未マージの場合）はユーザーに確認してから `-D` を使用:
```bash
git branch -D <name>
```
