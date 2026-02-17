# 01: ContextBase — 全HOT inline化 + DT-only新規追加

## 対象ファイル
- `source/engine/RHI/Public/IRHICommandContextBase.h`

## virtual → inline 変換（14個）

### Barrier系（4個）
| メソッド | DispatchTableエントリ | デフォルト引数 |
|---------|---------------------|--------------|
| `TransitionResource(resource, before, after)` | `TransitionResource` | — |
| `UAVBarrier(resource)` | `UAVBarrier` | resource = nullptr |
| `AliasingBarrier(before, after)` | `AliasingBarrier` | — |
| `FlushBarriers()` | `FlushBarriers` | — |

### Copy系（6個）
| メソッド | DispatchTableエントリ | デフォルト引数 |
|---------|---------------------|--------------|
| `CopyBuffer(dst, src)` | `CopyBuffer` | — |
| `CopyBufferRegion(dst, dstOff, src, srcOff, size)` | `CopyBufferRegion` | — |
| `CopyTexture(dst, src)` | `CopyTexture` | — |
| `CopyTextureRegion(dst, dstMip, dstSlice, dstOff, src, srcMip, srcSlice, srcBox)` | `CopyTextureRegion` | srcBox = nullptr |
| `CopyBufferToTexture(...)` | `CopyBufferToTexture` | — |
| `CopyTextureToBuffer(...)` | `CopyTextureToBuffer` | srcBox = nullptr |

### Debug+Breadcrumb（4個）
| メソッド | DispatchTableエントリ | デフォルト引数 |
|---------|---------------------|--------------|
| `BeginDebugEvent(name, color)` | `BeginDebugEvent` | color = 0 |
| `EndDebugEvent()` | `EndDebugEvent` | — |
| `InsertDebugMarker(name, color)` | `InsertDebugMarker` | color = 0 |
| `InsertBreadcrumb(id, message)` | `InsertBreadcrumb` | message = nullptr |

## DT-only → 新規 inline 追加（3個）

DispatchTableにエントリがあるがvirtual宣言が存在しない。IRHIUploadContextパターンで新規追加。

| メソッド | DispatchTableエントリ | 備考 |
|---------|---------------------|------|
| `ResolveTexture(dst, src)` | `ResolveTexture` | 新規追加 |
| `ResolveTextureRegion(dst, dstMip, dstSlice, src, srcMip, srcSlice)` | `ResolveTextureRegion` | 新規追加 |
| `CopyToStagingBuffer(dst, dstOff, src, srcOff, size)` | `CopyToStagingBuffer` | 新規追加。第1引数型 `IRHIStagingBuffer*` |

## パターン
```cpp
// Before:
virtual void TransitionResource(IRHIResource* resource, ERHIAccess before, ERHIAccess after) = 0;

// After:
void TransitionResource(IRHIResource* resource, ERHIAccess before, ERHIAccess after)
{
    NS_RHI_DISPATCH(TransitionResource, this, resource, before, after);
}
```

## TODO
- [ ] Barrier 4個 virtual → inline 変換
- [ ] Copy 6個 virtual → inline 変換
- [ ] Debug+Breadcrumb 4個 virtual → inline 変換
- [ ] ResolveTexture, ResolveTextureRegion, CopyToStagingBuffer を非virtual inlineとして新規追加
- [ ] ビルド確認
