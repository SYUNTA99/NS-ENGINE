# 13: DispatchTable（コアエントリ）

## 目的
Phase 1で必要な主要DispatchTableエントリをD3D12に接続する。

## 参照
- source/engine/RHI/Public/RHIDispatchTable.h
- source/engine/RHI/Private/RHIDispatchTable.cpp

## TODO
- [ ] Phase 1必須エントリのD3D12_Xxx関数実装 + GRHIDispatchTable登録
- [ ] 描画系: Draw, DrawIndexed, DrawIndirect, DrawIndexedIndirect, SetPrimitiveTopology
- [ ] 状態設定系: SetGraphicsPipelineState, SetComputePipelineState, SetGraphicsRootSignature, SetComputeRootSignature, SetVertexBuffers, SetIndexBuffer
- [ ] RT/DS系: SetRenderTargets, ClearRenderTargetView, ClearDepthStencilView, SetViewports, SetScissorRects, SetBlendFactor, SetStencilRef
- [ ] その他コア: Begin, Finish, Reset, Dispatch, SetDescriptorHeaps, SetGraphicsRootDescriptorTable, SetGraphicsRoot32BitConstants, SetGraphicsRootCBV, WriteTimestamp

## 対象外（50-51で実装）
- RT系: DispatchRays, BuildAccelerationStructure等
- Mesh系: DispatchMesh, SetMeshPipelineState等
- WG系: DispatchGraph, SetWorkGraphPipeline等
- VRS系: SetShadingRate, SetShadingRateImage

## 完了条件
- 上記エントリがD3D12に接続、三角形描画パスで呼出し確認

## 見積もり
- 新規ファイル: 2 (D3D12Dispatch.h/.cpp)
