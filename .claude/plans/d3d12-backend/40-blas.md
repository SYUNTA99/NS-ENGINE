# 40: BLAS（Bottom-Level Acceleration Structure）

## 目的
DXR 1.1 BLAS構築。Bindless(34)は不要、Phase 1基盤(28)があれば実装可能。

## 参照
- docs/D3D12RHI/D3D12RHI_Part09_RayTracing.md §BLAS
- source/engine/RHI/Public/RHIRaytracingAS.h

## 依存
- 28（Phase 1完了: Resource/Descriptor/CommandContext基盤）

## TODO
- [ ] D3D12RayTracingGeometry.h/.cpp — BLAS管理
- [ ] D3D12_RAYTRACING_GEOMETRY_DESC作成
- [ ] BuildRaytracingAccelerationStructure (BLAS)
- [ ] コンパクション: PostBuildInfo → **SyncPoint待機必須** → CompactedSize読取 → CopyRaytracingAccelerationStructure (COMPACT)
- [ ] SRV作成（BLAS参照用）

## 制約
- コンパクション同期: PostBuildInfoBufferReadbackのSyncPoint完了前にCompactedSize読取禁止（不正サイズ→バッファオーバーフロー）
- コンパクション適格条件: bAllowCompaction && !FastBuild && !AllowUpdate（Update有効時はコンパクション不可）
- Build前にUnregisterAsRenameListener、Build後にRegisterAsRenameListener

## 完了条件
- BLAS構築成功、コンパクション動作（SyncPoint同期正確）

## 見積もり
- 新規ファイル: 2 (D3D12RayTracingGeometry.h/.cpp)
