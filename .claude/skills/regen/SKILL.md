---
description: "Visual Studioソリューションを再生成"
model: haiku
allowed-tools:
  - "Bash(tools\\@regen_project.cmd:*)"
---

Premake5でVisual Studioソリューションを再生成します。

**実行コマンド**（Bashツールでそのまま実行）:
```
powershell -NoProfile -Command "Set-Location 'C:\Users\nanat\Desktop\NS-ENGINE'; & cmd /c 'tools\@regen_project.cmd' 2>&1"
```
