---
paths:
  - "Source/**"
  - "premake5.lua"
  - "tools/**"
---

# ビルド実行コマンド

Windowsコマンドは `powershell -Command` 経由で実行する。

## ビルド

```
powershell -Command "& cmd /c 'tools\@build.cmd Debug'"
powershell -Command "& cmd /c 'tools\@build.cmd Release'"
powershell -Command "& cmd /c 'tools\@build.cmd Burst'"
```

引数なしはDebug。timeout: 300000ms

## プロジェクト再生成

```
powershell -Command "& cmd /c 'tools\@regen_project.cmd'"
```

premake5.luaを変更した後、またはファイル追加・削除後に実行。

## テスト

```
powershell -Command "& cmd /c 'tools\@run_tests.cmd'"
```

## クリーンアップ

```
powershell -Command "& cmd /c 'tools\@cleanup.cmd'"
```

build/ディレクトリを削除。

## ビルドエラー時

1. エラーメッセージを読む
2. 該当ファイルを修正
3. 再ビルド（フルリビルドは不要、インクリメンタルビルドで十分）
