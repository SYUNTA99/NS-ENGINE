# 44: Work Graph State Object + バッキングメモリ

## 目的
D3D12 Work GraphsのState Object構築とバッキングメモリ管理。Phase 1基盤(28)があれば実装可能。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §WorkGraphs
- source/engine/RHI/Public/IRHIWorkGraph.h

## 依存
- 28（Phase 1完了: Resource/Descriptor/CommandContext基盤）

## TODO
- [ ] D3D12WorkGraph.h/.cpp — D3D12WorkGraphPipelineState
- [ ] ID3D12StateObject (EXECUTABLE) + ノード登録
- [ ] プロパティ取得: ProgramIdentifier + メモリ要件
- [ ] バッキングメモリ割り当て（65536アライメント、UAV）
- [ ] Feature検出: D3D12_FEATURE_D3D12_OPTIONS21

## 完了条件
- Work Graph State Object作成、バッキングメモリ確保

## 見積もり
- 新規ファイル: 2 (D3D12WorkGraph.h/.cpp)
