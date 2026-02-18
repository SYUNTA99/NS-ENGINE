# 46: Work Graph Binder

## 目的
Work Graphのシェーダーバンドルバインダーと並列バインディング。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §Binder

## TODO
- [ ] FWorkGraphShaderBundleBinder: ノード毎リソースバインド状態
- [ ] 並列RecordBindings（最大4ワーカー）
- [ ] FShaderBundleBinderOps: 遷移重複排除 + ワーカーマージ
- [ ] FD3D12BindlessConstantSetter

## 完了条件
- 並列RecordBindings動作

## 見積もり
- 新規ファイル: 2 (D3D12WorkGraphBinder.h/.cpp)
