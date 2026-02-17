# 02: ComputeContext — 全HOT inline化

## 対象ファイル
- `source/engine/RHI/Public/IRHIComputeContext.h`

## 変換する関数（17個）

### Pipeline+Bind（8個）
| メソッド | DispatchTableエントリ | 備考 |
|---------|---------------------|------|
| `SetComputePipelineState(pso)` | `SetComputePipelineState` | |
| `SetComputeRootSignature(rootSig)` | `SetComputeRootSignature` | |
| `SetComputeRoot32BitConstants(...)` | `SetComputeRoot32BitConstants` | destOffset = 0 |
| `SetComputeRootConstantBufferView(idx, addr)` | `SetComputeRootCBV` | **名前差異** |
| `SetComputeRootShaderResourceView(idx, addr)` | `SetComputeRootSRV` | **名前差異** |
| `SetComputeRootUnorderedAccessView(idx, addr)` | `SetComputeRootUAV` | **名前差異** |
| `SetDescriptorHeaps(cbvSrvUav, sampler)` | `SetDescriptorHeaps` | samplerHeap = nullptr |
| `SetComputeRootDescriptorTable(idx, handle)` | `SetComputeRootDescriptorTable` | |

### Dispatch（3個）
| メソッド | DispatchTableエントリ | デフォルト引数 |
|---------|---------------------|--------------|
| `Dispatch(x, y, z)` | `Dispatch` | — |
| `DispatchIndirect(buf, off)` | `DispatchIndirect` | argsOffset = 0 |
| `DispatchIndirectMulti(buf, off, count, stride)` | `DispatchIndirectMulti` | stride = 0 |

### UAVクリア（2個）
| メソッド | DispatchTableエントリ |
|---------|---------------------|
| `ClearUnorderedAccessViewUint(uav, values)` | `ClearUnorderedAccessViewUint` |
| `ClearUnorderedAccessViewFloat(uav, values)` | `ClearUnorderedAccessViewFloat` |

### Timestamp+Query（4個）
| メソッド | DispatchTableエントリ |
|---------|---------------------|
| `WriteTimestamp(heap, index)` | `WriteTimestamp` |
| `BeginQuery(heap, index)` | `BeginQuery` |
| `EndQuery(heap, index)` | `EndQuery` |
| `ResolveQueryData(heap, start, num, buf, off)` | `ResolveQueryData` |

## 名前マッピング注意
```cpp
void SetComputeRootConstantBufferView(uint32 rootParameterIndex, uint64 bufferAddress)
{
    NS_RHI_DISPATCH(SetComputeRootCBV, this, rootParameterIndex, bufferAddress);
}
```

## virtual維持（WARM）
- `GetCBVSRVUAVHeap()` — フレームあたり数回のステート問い合わせ
- `GetSamplerHeap()` — 同上

## DispatchTable-only（virtual宣言なし、変換不要）
- `GetQueryResult` — wait可能性あり。CmdXxx/switchからのみ使用。

## TODO
- [ ] Pipeline+Bind 8個 inline化（名前差異3箇所注意）
- [ ] Dispatch 3個 inline化（デフォルト引数維持）
- [ ] UAVクリア 2個 inline化
- [ ] Timestamp+Query 4個 inline化
- [ ] テンプレート便利関数 `SetComputeRootConstants<T>()`, `Dispatch1D()`, `Dispatch2D()` は非virtualのまま維持確認
- [ ] ビルド確認
