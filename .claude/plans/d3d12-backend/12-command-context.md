# 12: CommandContext

## 目的
IRHICommandContextのD3D12実装とRHICommandBufferからの変換。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md
- source/engine/RHI/Public/IRHICommandContext.h
- source/engine/RHI/Internal/RHICommandBuffer.h

## TODO
- [ ] D3D12CommandContext.h/.cpp 作成
- [ ] IRHICommandContext / IRHIComputeContext 実装
- [ ] RHICommandBuffer → ID3D12GraphicsCommandList変換（switchディスパッチ）
- [ ] ステートトラッキング: 現在のPSO、RootSignature、頂点バッファ等
- [ ] ExecuteIndirect: D3D12Device(05)のキャッシュ済みCommandSignatureを参照して間接描画実行

## 完了条件
- RHICommandBufferのswitchディスパッチがD3D12 APIを呼び出す

## 見積もり
- 新規ファイル: 2 (D3D12CommandContext.h/.cpp)
