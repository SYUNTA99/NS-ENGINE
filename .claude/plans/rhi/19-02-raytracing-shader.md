# 19-02: レイトレーシングシェーダー・SBT

## 状態: implemented

## 概要
シェーダー識別子、ヒットグループ、シェーダーバインディングテーブル（SBT）の管理インターフェースを定義する。

## 依存
- 19-01（加速構造）
- Part 06（シェーダー: IRHIShader, RHIShaderBytecode）
- Part 03（バッファ: IRHIBuffer）

## ファイル
- `source/engine/RHI/Public/RHIRaytracingShader.h`

## 既存基盤（実装済み）
| 型 | ファイル |
|---|---------|
| `EShaderFrequency::RayGen/RayMiss/...` | RHIEnums.h |
| `EShaderStageFlags::AllRayTracing` | RHIEnums.h |
| `ERHIResourceType::ShaderBindingTable` | IRHIResource.h |
| `RHIDispatchRaysArguments` | RHIIndirectArguments.h |

## 定数
| 名前 | 値 | 説明 |
|------|---|------|
| `kShaderIdentifierSize` | 32 | D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES |
| `kShaderRecordAlignment` | 32 | D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT |
| `kShaderTableAlignment` | 64 | D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT |

## 定義型一覧

### 構造体
| 型 | 説明 |
|---|------|
| `RHIShaderIdentifier` | 32バイトシェーダー識別子 |
| `RHIShaderRecord` | identifier + localRootArguments (アライメント付きサイズ計算) |
| `RHIHitGroupDesc` | hitGroupName, closestHit/anyHit/intersectionシェーダー名 |
| `RHIShaderTableRegion` | startAddress, size, stride (DispatchRays用) |
| `RHIShaderBindingTableDesc` | 各シェーダー種別のレコード数 + maxLocalRootArgumentsSize |
| `RHIDispatchRaysDesc` | 4つのShaderTableRegion + width/height/depth |

### インターフェース
| 型 | 継承 | 説明 |
|---|------|------|
| `IRHIShaderBindingTable` | IRHIResource | DECLARE_RHI_RESOURCE_TYPE(ShaderBindingTable) |
| `RHIShaderBindingTableRef` | TRefCountPtr | スマートポインタ |

### IRHIShaderBindingTableメソッド
```cpp
virtual RHIShaderTableRegion GetRayGenRegion() const = 0;
virtual RHIShaderTableRegion GetMissRegion() const = 0;
virtual RHIShaderTableRegion GetHitGroupRegion() const = 0;
virtual RHIShaderTableRegion GetCallableRegion() const = 0;
virtual void SetRayGenRecord(uint32 index, const RHIShaderRecord& record) = 0;
virtual void SetMissRecord(uint32 index, const RHIShaderRecord& record) = 0;
virtual void SetHitGroupRecord(uint32 index, const RHIShaderRecord& record) = 0;
virtual void SetCallableRecord(uint32 index, const RHIShaderRecord& record) = 0;
virtual IRHIBuffer* GetBuffer() const = 0;
virtual uint64 GetTotalSize() const = 0;
```

### RHIDispatchRaysDesc便利関数
```cpp
static RHIDispatchRaysDesc FromSBT(IRHIShaderBindingTable* sbt, uint32 w, uint32 h, uint32 d = 1);
```

### IRHIDevice追加メソッド
```cpp
virtual IRHIShaderBindingTable* CreateShaderBindingTable(
    const RHIShaderBindingTableDesc& desc, const char* debugName = nullptr) = 0;
```

## 設計ノート
- SBTはGPUバッファ上に配置され、DispatchRaysで参照される
- 各レコードは kShaderRecordAlignment (32バイト) アライメント
- テーブル全体は kShaderTableAlignment (64バイト) アライメント
- RayGenテーブルは常に1レコード（DXR仕様）だが、将来のDXR拡張に備え配列対応
