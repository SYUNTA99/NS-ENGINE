# 21: Sampler管理

## 目的
サンプラー作成とキャッシュ。

## 参照
- docs/D3D12RHI/D3D12RHI_Part07_Descriptors.md §Sampler

## TODO
- [ ] CreateSampler + サンプラーヒープ管理
- [ ] 静的サンプラー（Root Signature組み込み）
- [ ] 動的サンプラーヒープ
- [ ] サンプラーキャッシュ（重複排除）

## 完了条件
- シェーダーからテクスチャサンプリング可能

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Descriptors, D3D12View
