# 07: View handle cache — SRV+UAV+CBV+RTV+DSV

## 対象ファイル
- `source/engine/RHI/Public/IRHIViews.h`

## IRHIShaderResourceView（3個 virtual → cached）

| メソッド | キャッシュメンバ |
|---------|---------------|
| `GetGPUHandle()` | `m_gpuHandle` (RHIGPUDescriptorHandle) |
| `GetCPUHandle()` | `m_cpuHandle` (RHICPUDescriptorHandle) |
| `GetBindlessIndex()` | `m_bindlessIndex` (BindlessIndex) |

virtual維持: GetDevice, GetResource, IsBufferView

## IRHIUnorderedAccessView（3個 virtual → cached）

| メソッド | キャッシュメンバ |
|---------|---------------|
| `GetGPUHandle()` | `m_gpuHandle` |
| `GetCPUHandle()` | `m_cpuHandle` |
| `GetBindlessIndex()` | `m_bindlessIndex` |

virtual維持: GetDevice, GetResource, IsBufferView, HasCounter, GetCounterResource, GetCounterOffset

## IRHIConstantBufferView（4個 virtual → cached）

| メソッド | キャッシュメンバ |
|---------|---------------|
| `GetGPUHandle()` | `m_gpuHandle` (RHIGPUDescriptorHandle) |
| `GetCPUHandle()` | `m_cpuHandle` (RHICPUDescriptorHandle) |
| `GetBindlessIndex()` | `m_bindlessIndex` (BindlessIndex) |
| `GetGPUVirtualAddress()` | `m_gpuVirtualAddress` (uint64) |

virtual維持: GetDevice, GetBuffer, GetOffset, GetSize

## IRHIRenderTargetView（1個 virtual → cached）

| メソッド | キャッシュメンバ |
|---------|---------------|
| `GetCPUHandle()` | `m_cpuHandle` (RHICPUDescriptorHandle) |

virtual維持: GetDevice, GetTexture, GetMipSlice, GetFirstArraySlice, GetArraySize, GetFormat

## IRHIDepthStencilView（1個 virtual → cached）

| メソッド | キャッシュメンバ |
|---------|---------------|
| `GetCPUHandle()` | `m_cpuHandle` (RHICPUDescriptorHandle) |

virtual維持: GetDevice, GetTexture, GetMipSlice, GetFirstArraySlice, GetArraySize, GetFormat, GetFlags

## パターン
```cpp
RHIGPUDescriptorHandle GetGPUHandle() const { return m_gpuHandle; }
protected:
    RHIGPUDescriptorHandle m_gpuHandle{};
```

## TODO
- [ ] SRV: 3 virtual → protected member + non-virtual getter
- [ ] UAV: 3 virtual → protected member + non-virtual getter
- [ ] CBV: 4 virtual → protected member + non-virtual getter
- [ ] RTV: 1 virtual → protected member + non-virtual getter
- [ ] DSV: 1 virtual → protected member + non-virtual getter
- [ ] 5クラスで命名規則・パターン統一
- [ ] ビルド確認
