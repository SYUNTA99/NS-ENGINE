---
description: "テストを実行"
---

引数に基づいてテストを実行してください。

**実行コマンド**（Bashツールでそのまま実行）:

引数なし または `debug` の場合:
```
powershell -NoProfile -Command "Set-Location 'C:\Users\nanat\Desktop\NS-ENGINE'; & cmd /c 'tools\@run_tests.cmd Debug' 2>&1"
```

`release` の場合:
```
powershell -NoProfile -Command "Set-Location 'C:\Users\nanat\Desktop\NS-ENGINE'; & cmd /c 'tools\@run_tests.cmd Release' 2>&1"
```

フィルタ指定 (例: `/test ECS`) の場合:
```
powershell -NoProfile -Command "Set-Location 'C:\Users\nanat\Desktop\NS-ENGINE'; & cmd /c 'tools\@build.cmd Debug && build\bin\Debug-windows-x86_64\tests\tests.exe --gtest_filter=ECS*' 2>&1"
```

timeout: 300000ms
