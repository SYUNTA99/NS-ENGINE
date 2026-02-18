# 41: TLAS + Feature検出

## 目的
DXR 1.1 TLAS構築とRT Feature検出。

## 参照
- docs/D3D12RHI/D3D12RHI_Part09_RayTracing.md §TLAS
- source/engine/RHI/Public/RHIRaytracingAS.h

## TODO
- [ ] D3D12RayTracingScene.h/.cpp — TLAS管理
- [ ] D3D12_RAYTRACING_INSTANCE_DESCバッファ構築
- [ ] BuildRaytracingAccelerationStructure (TLAS) + 更新
- [ ] D3D12_FEATURE_D3D12_OPTIONS5 (RaytracingTier) 判定

## 完了条件
- TLAS構築成功、RHIフロントエンド接続

## 見積もり
- 新規ファイル: 2 (D3D12RayTracingScene.h/.cpp)
