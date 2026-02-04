# Investigation Plan: 独自Result型の調査

## Mode
investigation

## Target Feature
エンジン内の独自Result型を `common/result` のResult型に置き換え

## Goal
`common/result` を使っていないエンジン機能で独自のResult型を使っている箇所を特定し、置き換え計画を立てる

## Current Phase
Phase 4 (報告・承認待ち)

## 調査チェックリスト

### Phase 1: 既存コードの理解
- [x] 関連するモジュール/ファイルを特定
- [x] 既存のパターンを把握
- [x] 依存関係を整理
- **Status:** complete

### Phase 2: 要件の明確化
- [x] 必要なAPI/機能を洗い出し
- [x] 制約・境界条件を特定
- [x] 性能要件を確認
- **Status:** complete

### Phase 3: 影響範囲の特定
- [x] 変更が必要なファイルを列挙
- [x] 破壊的変更の有無を判断
- [x] テスト対象を特定
- **Status:** complete

### Phase 4: 報告・承認
- [x] 調査結果をユーザーに報告
- [ ] サブ計画への分割案を提示
- [ ] ユーザー承認
- **Status:** in_progress

## 調査中の疑問

1. FileReadResult の bytes メンバをどう扱うか？
   → 回答: Result<T> パターンまたは出力引数で分離

2. JobResult は置き換え対象か？
   → 回答: 対象外（状態を表すenumであり、Result型の概念と異なる）

## 発見事項サマリー

### 独自Result型
- **FileSystem**: FileError, FileReadResult, FileOperationResult, AsyncReadState
- **JobSystem**: JobResult（対象外：状態enum）

### boolを返す関数パターン
- Graphics: Initialize(), CreateShaders() 等
- Platform: Renderer::Initialize(), Window::CreateWindowInternal() 等
- Texture: ITextureLoader::Load() 等
- Engine Core: Engine::Initialize() 等

### 優先度
1. **高**: FileSystem（独自Result型が最も発達）
2. **中**: Graphics初期化、Texture読み込み
3. **低**: Platform初期化
4. **対象外**: JobResult

## 推奨サブ計画案

| # | サブ計画 | 概要 |
|---|----------|------|
| 1 | FileSystemResult拡張 | 不足エラーコード追加、変換ユーティリティ作成 |
| 2 | FileError置き換え | FileError → NS::Result 移行 |
| 3 | FileReadResult置き換え | FileReadResult → Result + outBytes 移行 |
| 4 | FileOperationResult置き換え | FileOperationResult → Result 移行 |
| 5 | (将来) Graphics初期化 | bool → Result 移行 |

## 次のステップ

- [x] 調査結果を findings.md に記録
- [ ] ユーザーに報告し、計画案を提示
- [ ] ユーザー承認後、`/plan-large-scale` で計画ファイル作成
- [ ] 実装フェーズへ

## Notes
- JobResult は置き換え対象外（状態enumであり、Result型とは異なる概念）
- FileSystem は common/result/Module/FileSystemResult.h と直接マッピング可能
- 段階的移行を推奨（影響範囲を限定）
