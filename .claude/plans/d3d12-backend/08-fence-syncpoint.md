# 08: Fence + SyncPoint

## 目的
ID3D12Fenceラッパーと RHISyncPoint のD3D12実装。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md §Fence
- source/engine/RHI/Public/IRHIFence.h
- source/engine/RHI/Public/RHISyncPoint.h

## TODO
- [ ] D3D12Fence.h/.cpp — IRHIFence実装 + 単調増加フェンス値管理
- [ ] CPU待機: SetEventOnCompletion + WaitForSingleObject
- [ ] ポーリング: GetCompletedValue + フェンスプール
- [ ] RHISyncPoint D3D12実装: Queue+FenceValueペア
- [ ] クロスキュー待機（Graphics → Copy完了待ち等）

## 完了条件
- GPU-CPU同期動作、キュー間同期動作

## 見積もり
- 新規ファイル: 2 (D3D12Fence.h/.cpp)
