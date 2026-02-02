---
description: "プロジェクトをフルリビルド"
allowed-tools:
  - "Bash(tools\\@make_project.cmd:*)"
---

フルリビルドを実行します（クリーン→生成→ビルド）。

**実行コマンド**（Bashツールでそのまま実行）:
```
powershell -NoProfile -Command "Set-Location 'C:\Users\nanat\Desktop\NS-ENGINE'; & cmd /c 'tools\@make_project.cmd' 2>&1"
```

timeout: 300000ms
