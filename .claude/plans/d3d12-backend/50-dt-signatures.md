# 50: DispatchTable関数シグネチャ定義

## 目的
全107+エントリのD3D12_Xxx関数を定義する。

## 参照
- source/engine/RHI/Public/RHIDispatchTable.h (L849-866)

## TODO
- [ ] 13で未実装のPhase 2/3エントリのD3D12_Xxx関数定義（残り約70エントリ）
- [ ] RT系: D3D12_DispatchRays, D3D12_BuildAccelerationStructure, D3D12_BindAccelerationStructureMemory等
- [ ] Mesh/WG/VRS系: D3D12_DispatchMesh, D3D12_DispatchGraph, D3D12_SetShadingRate等

## 境界
- 13で登録済み: Phase 1コアエントリ（Draw/Dispatch/SetPSO/SetRT等 約35エントリ）
- 本サブ計画: Phase 2/3の残り全エントリ（RT/Mesh/WG/VRS/Upload拡張等）

## 完了条件
- 全107+エントリの関数シグネチャ定義完了、RHIDispatchTable型と一致

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Dispatch.h/.cpp
