# 30: Enhanced Barrier

## 目的
D3D12_BARRIER_xxx ベースのEnhanced Barrierシステム（FL 12.2+）。

## 参照
- docs/D3D12RHI/D3D12RHI_Part06_Barriers.md §Enhanced

## TODO
- [ ] D3D12_BARRIER_GROUP構築
- [ ] D3D12_TEXTURE_BARRIER / BUFFER_BARRIER / GLOBAL_BARRIER
- [ ] D3D12_BARRIER_SYNC / D3D12_BARRIER_ACCESS + Layout互換性テーブル
- [ ] Feature判定: CheckFeatureSupport → Enhanced有効時切替
- [ ] プラットフォーム固有ワークアラウンド（下記制約参照）

## 制約
- NVIDIA: GFX-only readビット（DSVRead, ShadingRateSource, ResolveSource）をComputeキューでも読取可能にするハック
- Windows: SYNC_RAYTRACING + ACCESS_ACCELERATION_STRUCTURE_WRITE の組合せ禁止
- DSV_WRITE layout + DSVRead access の組合せ禁止（一部ドライバ）
- ComputeWrite→ComputeWriteバリアはキャッシュ独立のため省略可能（BarrierCanBeDiscarded）

## 完了条件
- Enhanced Barrier動作（対応GPU）、プラットフォームワークアラウンド適用

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Barriers
