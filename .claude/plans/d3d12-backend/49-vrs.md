# 49: Variable Rate Shading

## 目的
VRS Tier 1/2の実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part10_WorkGraphsMeshVRS.md §VRS
- source/engine/RHI/Public/RHIVariableRateShading.h

## TODO
- [ ] Tier 1: RSSetShadingRate + D3D12_SHADING_RATE_COMBINER
- [ ] Tier 2: SV_ShadingRateセマンティクス + シェーディングレートイメージ
- [ ] Feature検出: D3D12_FEATURE_D3D12_OPTIONS6 + タイルサイズ取得
- [ ] DispatchTable登録 + HasVariableRateShadingSupport() = true

## 完了条件
- VRS Tier 1/2動作

## 見積もり
- 新規ファイル: 2 (D3D12VRS.h/.cpp)
