# 08: Buffer — accessor キャッシュ化

## 対象ファイル
- `source/engine/RHI/Public/IRHIBuffer.h`

## 変換する関数（3個）

| メソッド | キャッシュメンバ | 型 |
|---------|---------------|-----|
| `GetGPUVirtualAddress()` | `m_gpuVirtualAddress` | `uint64` |
| `GetSize()` | `m_size` | `MemorySize` |
| `GetStride()` | `m_stride` | `uint32` |

## virtual維持
- `GetDevice()` — COLD
- `GetUsage()` — COLD
- `GetMemoryInfo()` — COLD
- `Map()`, `Unmap()`, `IsMapped()` — WARM
- `IsBuffer()` override — COLD

## パターン
```cpp
uint64 GetGPUVirtualAddress() const { return m_gpuVirtualAddress; }
MemorySize GetSize() const { return m_size; }
uint32 GetStride() const { return m_stride; }
protected:
    uint64 m_gpuVirtualAddress = 0;
    MemorySize m_size = 0;
    uint32 m_stride = 0;
```

## TODO
- [ ] 3個の virtual → protected member + non-virtual getter
- [ ] protected member のレイアウト（アラインメント考慮: uint64×2 + uint32）
- [ ] GetElementCount() 等の既存 non-virtual メソッドが正常動作すること確認
- [ ] ビルド確認
