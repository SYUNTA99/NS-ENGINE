# 32: Buddy/Pool アロケータ

## 目的
Placed Resourceを活用したBuddy/Poolアロケータ。

## 参照
- docs/D3D12RHI/D3D12RHI_Part05_MemoryAllocation.md

## TODO
- [ ] Buddyアロケータ: 2のべき乗分割 + 隣接フリーブロック再帰マージ（断片化防止）
- [ ] ID3D12Heap上のPlaced Resource配置
- [ ] Poolアロケータ: 固定サイズブロックプール + DeferredDeletionQueue（フレームフェンス連動）
- [ ] スレッドセーフ: Buddy→RWLock(Read:Allocate, Write:Deallocate)、Pool→CriticalSection

## 制約
- Placed Resource最小サイズ: 64KB（D3D12制約）
- Deferred解放: フレームフェンス完了値を確認後にのみ実解放（GPU使用中のブロック保護）
- MaxAllocationSize超過時: Committed Resourceへフォールバック（Placed不可）

## 完了条件
- Placed Resource経由のバッファ/テクスチャ作成、スレッドセーフ動作

## 見積もり
- 新規ファイル: 2 (D3D12Allocation.h/.cpp)
