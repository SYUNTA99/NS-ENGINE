---
description: "C++コードの静的解析と自動修正 (clang-tidy)"
---

# C++ Lint (clang-tidy)

`$ARGUMENTS` が指定された場合はそのファイル/ディレクトリを対象にします。
指定がない場合は `source/` 全体を対象にします。

## 設定ファイル

- `.claude/clang-tidy.yaml`: clang-tidy 設定
- `premake/compile_commands.json`: コンパイルコマンドDB

## 実行方法

### 単一ファイルの場合

```bash
clang-tidy -p premake --config-file=.claude/clang-tidy.yaml <file.cpp>
```

### ディレクトリ全体の場合

clang-tidy はディレクトリを直接受け付けない。run-clang-tidy を使用:

```bash
python "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin/run-clang-tidy" \
  -p premake \
  -config-file=.claude/clang-tidy.yaml \
  "source/engine/.*"
```

または find + xargs:

```bash
find source/engine -name "*.cpp" | xargs clang-tidy -p premake --config-file=.claude/clang-tidy.yaml
```

## 出力フォーマット

### 検出された問題
| ファイル | 行 | カテゴリ | 問題 |
|---------|---|---------|------|

### 修正サマリー
- 警告: N件
- エラー: N件

## 自動修正 (--fix)

**重要**: --fix は確認なしで大量のファイルを書き換える可能性がある。

1. まず --fix なしで問題一覧を表示
2. ユーザーに修正対象を確認
3. 承認後のみ --fix を実行

```bash
# 修正実行（承認後のみ）
clang-tidy -p premake --config-file=.claude/clang-tidy.yaml --fix <file.cpp>
```
