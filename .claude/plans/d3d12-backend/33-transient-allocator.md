# 33: Transientアロケータ + RHI接続

## 目的
フレーム単位の一時リソースアロケータとRHIフロントエンド接続。

## 参照
- docs/D3D12RHI/D3D12RHI_Part05_MemoryAllocation.md §Transient
- source/engine/RHI/Public/RHITransientAllocator.h

## TODO
- [ ] フレーム単位リニアアロケータ
- [ ] Upload Heapリングバッファ
- [ ] 1フレーム寿命の一時リソース
- [ ] RHIBufferAllocator → D3D12 Buddy/Pool接続
- [ ] RHITransientBufferPool → D3D12 Transient接続

## 完了条件
- 一時バッファのフレーム間再利用、メモリ使用量統計取得

## 見積もり
- 新規ファイル: 0
- 変更ファイル: D3D12Allocation
