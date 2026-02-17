# 06: GraphicsContext — Mesh+Raytracing+WorkGraph+Misc inline化

## 対象ファイル
- `source/engine/RHI/Public/IRHICommandContext.h`

## 変換する関数（14個）

### メッシュシェーダー（4個）
| メソッド | DispatchTableエントリ | デフォルト引数 |
|---------|---------------------|--------------|
| `SetMeshPipelineState(pso)` | `SetMeshPipelineState` | — |
| `DispatchMesh(x, y, z)` | `DispatchMesh` | y = 1, z = 1 |
| `DispatchMeshIndirect(argBuf, argOff)` | `DispatchMeshIndirect` | argumentOffset = 0 |
| `DispatchMeshIndirectCount(argBuf, argOff, countBuf, countOff, max)` | `DispatchMeshIndirectCount` | — |

### レイトレーシング（4個）
| メソッド | DispatchTableエントリ | 参照→ポインタ |
|---------|---------------------|-------------|
| `BuildRaytracingAccelerationStructure(desc)` | `BuildRaytracingAccelerationStructure` | `const RHIAccelerationStructureBuildDesc&` → `&desc` |
| `CopyRaytracingAccelerationStructure(dest, src, mode)` | `CopyRaytracingAccelerationStructure` | — |
| `SetRaytracingPipelineState(pso)` | `SetRaytracingPipelineState` | — |
| `DispatchRays(desc)` | `DispatchRays` | `const RHIDispatchRaysDesc&` → `&desc` |

### ワークグラフ（3個）
| メソッド | DispatchTableエントリ | 参照→ポインタ |
|---------|---------------------|-------------|
| `SetWorkGraphPipeline(pipeline)` | `SetWorkGraphPipeline` | — |
| `DispatchGraph(desc)` | `DispatchGraph` | `const RHIWorkGraphDispatchDesc&` → `&desc` |
| `InitializeWorkGraphBackingMemory(pipeline, memory)` | `InitializeWorkGraphBackingMemory` | `const RHIWorkGraphBackingMemory&` → `&memory` |

### その他（3個）
| メソッド | DispatchTableエントリ | デフォルト引数 |
|---------|---------------------|--------------|
| `ExecuteIndirect(cmdSig, maxCount, argBuf, argOff, countBuf, countOff)` | `ExecuteIndirect` | countBuffer = nullptr, countOffset = 0 |
| `BeginBreadcrumbGPU(node)` | `BeginBreadcrumbGPU` | — |
| `EndBreadcrumbGPU(node)` | `EndBreadcrumbGPU` | — |

## TODO
- [ ] Mesh 4個 inline化（デフォルト引数維持）
- [ ] Raytracing 4個 inline化（参照→ポインタ変換2箇所注意）
- [ ] WorkGraph 3個 inline化（参照→ポインタ変換2箇所注意）
- [ ] ExecuteIndirect+Breadcrumb 3個 inline化（デフォルト引数維持）
- [ ] ビルド確認
