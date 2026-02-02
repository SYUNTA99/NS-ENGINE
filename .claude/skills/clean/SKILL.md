---
description: "ビルド成果物をクリーンアップ"
model: haiku
allowed-tools:
  - "Bash(tools\\@cleanup.cmd:*)"
---

ビルド成果物を削除します（`build/` と `.vs/`）。

**実行コマンド**（Bashツールでそのまま実行）:
```
powershell -NoProfile -Command "Set-Location 'C:\Users\nanat\Desktop\NS-ENGINE'; & cmd /c 'tools\@cleanup.cmd' 2>&1"
```
