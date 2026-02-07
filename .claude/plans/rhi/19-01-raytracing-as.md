# 19-01: レイトレーシング加速構造

## 状態: implemented

## 概要
DXR 1.1準拠の加速構造（BLAS/TLAS）の構築・更新インターフェースを定義する。

## 依存
- Part 03（バッファ: IRHIBuffer）
- Part 02（コマンドコンテキスト: IRHICommandContext）
- Part 01（リソース基底: IRHIResource, DECLARE_RHI_RESOURCE_TYPE）

## ファイル
- `source/engine/RHI/Public/RHIRaytracingAS.h`

## 既存基盤（実装済み）
| 型 | ファイル |
|---|---------|
| `ERHIAccess::AccelerationStructureRead/Build` | RHIEnums.h |
| `ERHIBufferUsage::AccelerationStructure` | RHIEnums.h |
| `ERHIResourceType::AccelerationStructure` | IRHIResource.h |
| `IsRayTracingShader()` | RHIEnums.h |
| `RHIAdapterDesc::supportsRayTracing` | RHIAdapterDesc.h |

## 定義型一覧

### 列挙型
| 型 | 基底 | 説明 |
|---|------|------|
| `ERHIRaytracingGeometryType` | uint8 | Triangles, ProceduralAABBs |
| `ERHIRaytracingGeometryFlags` | uint32 | Opaque, NoDuplicateAnyHit (ビットフラグ) |
| `ERHIRaytracingInstanceFlags` | uint32 | TriangleCullDisable, ForceOpaque等 (ビットフラグ) |
| `ERHIRaytracingAccelerationStructureType` | uint8 | TopLevel, BottomLevel |
| `ERHIRaytracingBuildFlags` | uint32 | AllowUpdate, PreferFastTrace等 (ビットフラグ) |
| `ERHIRaytracingCopyMode` | uint8 | Clone, Compact, Serialize, Deserialize |
| `ERHIRaytracingTier` | uint8 | NotSupported, Tier1_0, Tier1_1 |

### 構造体
| 型 | 説明 |
|---|------|
| `RHIRaytracingGeometryTrianglesDesc` | 頂点/インデックスバッファGPUアドレス、フォーマット |
| `RHIRaytracingGeometryAABBsDesc` | AABBバッファGPUアドレス、ストライド |
| `RHIRaytracingGeometryDesc` | ジオメトリタイプ + triangles/aabbs union |
| `RHIRaytracingInstanceDesc` | 3x4 Transform + InstanceID/Mask + BLAS参照 (D3D12互換) |
| `RHIRaytracingAccelerationStructureBuildInputs` | BLAS: geometries配列, TLAS: instanceDescsAddress |
| `RHIRaytracingAccelerationStructurePrebuildInfo` | resultDataMaxSize, scratchDataSize |
| `RHIRaytracingAccelerationStructureDesc` | 作成記述 (resultBuffer + GPUMask) |
| `RHIRaytracingCapabilities` | tier, maxInstanceCount, maxRecursionDepth等 |
| `RHIAccelerationStructureBuildDesc` | ビルドコマンド記述 (inputs + dest + scratch) |

### インターフェース
| 型 | 継承 | 説明 |
|---|------|------|
| `IRHIAccelerationStructure` | IRHIResource | DECLARE_RHI_RESOURCE_TYPE(AccelerationStructure) |
| `RHIAccelerationStructureRef` | TRefCountPtr | スマートポインタ |

### IRHIAccelerationStructureメソッド
```cpp
virtual uint64 GetGPUVirtualAddress() const = 0;
virtual IRHIBuffer* GetResultBuffer() const = 0;
virtual uint64 GetResultBufferOffset() const = 0;
virtual uint64 GetSize() const = 0;
```

### IRHIDevice追加メソッド
```cpp
virtual IRHIAccelerationStructure* CreateAccelerationStructure(
    const RHIRaytracingAccelerationStructureDesc& desc, const char* debugName = nullptr) = 0;
virtual RHIRaytracingAccelerationStructurePrebuildInfo GetAccelerationStructurePrebuildInfo(
    const RHIRaytracingAccelerationStructureBuildInputs& inputs) const = 0;
virtual RHIRaytracingCapabilities GetRaytracingCapabilities() const = 0;
```

### IRHICommandContext追加メソッド
```cpp
virtual void BuildRaytracingAccelerationStructure(const RHIAccelerationStructureBuildDesc& desc) = 0;
virtual void CopyRaytracingAccelerationStructure(IRHIAccelerationStructure* dest,
    IRHIAccelerationStructure* source, ERHIRaytracingCopyMode mode) = 0;
```

## DXR 1.1対応
- `ERHIRaytracingTier::Tier1_1`: インラインレイトレーシング（RayQuery in compute/pixel shaders）
- `ERHIRaytracingBuildFlags::PerformUpdate`: インプレース更新
- `ERHIRaytracingCopyMode::Compact`: ビルド後のメモリ圧縮
