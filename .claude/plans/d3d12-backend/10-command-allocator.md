# 10: CommandAllocator

## 目的
D3D12のCommandAllocatorプール管理。

## 参照
- docs/D3D12RHI/D3D12RHI_Part03_CommandExecution.md
- source/engine/RHI/Public/IRHICommandAllocator.h

## TODO
- [ ] D3D12CommandAllocator.h/.cpp 作成
- [ ] IRHICommandAllocator実装
- [ ] タイプ別プール: DIRECT/COMPUTE/COPY
- [ ] フレーム遅延リセット（GPU完了後にReset）

## 完了条件
- Allocator取得→Reset→再利用サイクル動作

## 見積もり
- 新規ファイル: 2 (D3D12CommandAllocator.h/.cpp)
