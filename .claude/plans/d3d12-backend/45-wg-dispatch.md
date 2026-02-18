# 45: Work Graph Dispatch

## 目的
Work GraphのCompute/Graphics Dispatch実行。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §Dispatch

## TODO
- [ ] Compute: SetProgram + DispatchGraph
- [ ] エントリシェーダー特定 + ルート引数テーブル構築
- [ ] ルート引数アップロード（コピーキュー経由）
- [ ] Graphics: ルートシグネチャ + ビューポート/シザー設定
- [ ] デュアルRecordBindings (SF_Pixel, SF_Mesh)

## 完了条件
- Compute DispatchGraph実行

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12WorkGraph, D3D12CommandContext
