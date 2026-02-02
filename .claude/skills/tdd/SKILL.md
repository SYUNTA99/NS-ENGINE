---
description: "テスト駆動開発ガイド（Red-Green-Refactor）"
---

# Task
TDDで機能を実装: $ARGUMENTS

1. 機能要件を分析
2. まず失敗するテストを書く (RED)
3. テストを通す最小限のコードを実装 (GREEN)
4. テストを維持しながらリファクタリング
5. テストファースト証拠付きでコミット

## テストの場所
- `source/tests/engine/` - エンジンテスト
- Google Test フレームワーク使用
- 実行: `/test` または `tools\@run_tests.cmd`
