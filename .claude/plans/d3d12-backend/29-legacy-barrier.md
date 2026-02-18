# 29: Legacy Barrier

## 目的
D3D12_RESOURCE_BARRIERベースのLegacyバリアシステム。

## 参照
- docs/D3D12RHI/D3D12RHI_Part06_Barriers.md §Legacy
- source/engine/RHI/Public/RHIBarrier.h

## TODO
- [ ] D3D12Barriers.h/.cpp 作成
- [ ] D3D12_RESOURCE_BARRIER (Transition/UAV/Aliasing)
- [ ] ResourceBarrierバッチ投入
- [ ] Split Barriers (BEGIN_ONLY / END_ONLY)
- [ ] 状態プロモーション/ディケイ規則

## 完了条件
- Legacy Barrier動作（全プラットフォーム）

## 見積もり
- 新規ファイル: 2 (D3D12Barriers.h/.cpp)
