# 06: IDynamicRHI接続

## 目的
D3D12DynamicRHIを完成させ、Adapter列挙→Device作成の初期化フローを接続する。

## 参照
- source/engine/RHI/Public/IDynamicRHI.h
- docs/D3D12RHI/D3D12RHI_Part01_Architecture.md §2

## TODO
- [ ] D3D12DynamicRHI.h/.cpp — IDynamicRHI完全実装
- [ ] Init() → Factory作成 → Adapter列挙 → Device作成 → Feature検出 → GIsRHIInitialized=true
- [ ] Shutdown() → 下記順序厳守: GIsRHIInitialized=false → リソース全解放 → BlockUntilIdle → キュー破棄 → デバイス破棄 → Factory破棄
- [ ] GetAdapterDesc() 実装
- [ ] GRHISupportsXxx主要フラグ設定（Feature検出結果に基づく）: MultiDrawIndirect, Multithreading, AsyncGetRenderQueryResult, ParallelRenderPasses, RawViewsForAnyBuffer, TimestampRenderQueries, ParallelOcclusionQueries, MapWriteNoOverwrite, RHIThread, RHIOnTaskThread

## 制約
- Init順序（厳守）: Module→IsSupported→FindAdapter→Constructor→Init→PostInit→GIsRHIInitialized=true
- Shutdown順序（厳守、Init逆順）: GIsRHIInitialized=false → DeferredDelete排出 → 全GPU待機(BlockUntilIdle) → Submission停止 → Adapter/Device/Queue破棄
- BlockUntilIdleをリソース解放前に呼ばないとGPUフォルト

## 完了条件
- Init/Shutdownサイクル正常動作、Shutdown時にGPUフォルトなし

## 見積もり
- 新規ファイル: 2 (D3D12DynamicRHI.h/.cpp)
