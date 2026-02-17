# 03: GraphicsContext — Draw+Pipeline+Bind inline化

## 対象ファイル
- `source/engine/RHI/Public/IRHICommandContext.h`

## 変換する関数（16個）

### Draw系（6個）
| メソッド | DispatchTableエントリ | デフォルト引数 |
|---------|---------------------|--------------|
| `Draw(vertexCount, instanceCount, startVertex, startInstance)` | `Draw` | instanceCount=1, startVertex=0, startInstance=0 |
| `DrawIndexed(indexCount, instanceCount, startIndex, baseVertex, startInstance)` | `DrawIndexed` | instanceCount=1, startIndex=0, baseVertex=0, startInstance=0 |
| `DrawIndirect(argsBuffer, argsOffset)` | `DrawIndirect` | argsOffset = 0 |
| `DrawIndexedIndirect(argsBuffer, argsOffset)` | `DrawIndexedIndirect` | argsOffset = 0 |
| `MultiDrawIndirect(argsBuffer, drawCount, argsOffset, argsStride)` | `MultiDrawIndirect` | argsOffset = 0, argsStride = 0 |
| `MultiDrawIndirectCount(argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, argsStride)` | `MultiDrawIndirectCount` | — |

### Pipeline+RootSignature（2個）
| メソッド | DispatchTableエントリ |
|---------|---------------------|
| `SetGraphicsPipelineState(pso)` | `SetGraphicsPipelineState` |
| `SetGraphicsRootSignature(rootSig)` | `SetGraphicsRootSignature` |

### VB/IB/Topology（3個）
| メソッド | DispatchTableエントリ | 備考 |
|---------|---------------------|------|
| `SetVertexBuffers(startSlot, numBuffers, views)` | `SetVertexBuffers` | |
| `SetIndexBuffer(view)` | `SetIndexBuffer` | 参照→ポインタ: `const RHIIndexBufferView&` → `&view` |
| `SetPrimitiveTopology(topology)` | `SetPrimitiveTopology` | |

### RootDescriptor+定数（5個）
| メソッド | DispatchTableエントリ | 備考 |
|---------|---------------------|------|
| `SetGraphicsRootDescriptorTable(idx, handle)` | `SetGraphicsRootDescriptorTable` | |
| `SetGraphicsRootConstantBufferView(idx, loc)` | `SetGraphicsRootCBV` | **名前差異** |
| `SetGraphicsRootShaderResourceView(idx, loc)` | `SetGraphicsRootSRV` | **名前差異** |
| `SetGraphicsRootUnorderedAccessView(idx, loc)` | `SetGraphicsRootUAV` | **名前差異** |
| `SetGraphicsRoot32BitConstants(idx, num, data, off)` | `SetGraphicsRoot32BitConstants` | destOffset = 0 |

## 非virtualラッパー（変更不要）
- `SetVertexBuffer(slot, view)` → SetVertexBuffers(slot, 1, &view)
- `SetGraphicsRootConstants<T>(idx, value)` → SetGraphicsRoot32BitConstants テンプレート

## TODO
- [ ] Draw 6個 inline化（デフォルト引数維持）
- [ ] PSO+RootSig 2個 inline化
- [ ] VB/IB/Topology 3個 inline化（SetIndexBuffer ref→ptr注意）
- [ ] RootDesc+定数 5個 inline化（名前差異3箇所注意）
- [ ] 非virtualラッパーの動作確認
- [ ] ビルド確認
