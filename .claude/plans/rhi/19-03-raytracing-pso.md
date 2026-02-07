# 19-03: レイトレーシングPSO

## 状態: implemented

## 概要
DXILライブラリベースのレイトレーシングパイプラインステートオブジェクト（RTPSO）を定義する。

## 依存
- 19-02（シェーダー・SBT: RHIShaderIdentifier, RHIHitGroupDesc）
- Part 08（ルートシグネチャ: IRHIRootSignature）
- Part 06（シェーダー: RHIShaderBytecode）

## ファイル
- `source/engine/RHI/Public/RHIRaytracingPSO.h`

## 既存基盤（実装済み）
| 型 | ファイル |
|---|---------|
| `ERHIResourceType::RayTracingPSO` | IRHIResource.h |
| `ERHIIndirectArgumentType::DispatchRays` | RHICommandSignature.h |
| メッシュPSOパターン | RHIMeshPipelineState.h (参考) |

## 定義型一覧

### 構造体
| 型 | 説明 |
|---|------|
| `RHIDXILLibraryDesc` | ライブラリバイトコード + エクスポート名リスト |
| `RHIRaytracingShaderConfig` | maxPayloadSize, maxAttributeSize |
| `RHIRaytracingPipelineConfig` | maxTraceRecursionDepth |
| `RHILocalRootSignatureAssociation` | ローカルルートシグネチャ→エクスポート関連付け |
| `RHIRaytracingPipelineStateDesc` | 全RTPSO構成要素を集約 |

### インターフェース
| 型 | 継承 | 説明 |
|---|------|------|
| `IRHIRaytracingPipelineState` | IRHIResource | DECLARE_RHI_RESOURCE_TYPE(RayTracingPSO) |
| `RHIRaytracingPipelineStateRef` | TRefCountPtr | スマートポインタ |

### IRHIRaytracingPipelineStateメソッド
```cpp
virtual RHIShaderIdentifier GetShaderIdentifier(const char* exportName) const = 0;
virtual uint32 GetMaxPayloadSize() const = 0;
virtual uint32 GetMaxAttributeSize() const = 0;
virtual uint32 GetMaxRecursionDepth() const = 0;
virtual IRHIRootSignature* GetGlobalRootSignature() const = 0;
```

### ビルダー
```cpp
class RHIRaytracingPipelineStateBuilder {
    AddLibrary(bytecode, exportNames, exportCount)
    AddHitGroup(name, closestHit, anyHit, intersection)
    SetShaderConfig(maxPayload, maxAttribute)
    SetMaxRecursionDepth(depth)
    SetGlobalRootSignature(rootSig)
    AddLocalRootSignature(rootSig, exports, count)
    SetDebugName(name)
    Build() -> const RHIRaytracingPipelineStateDesc&
};
```

### IRHIDevice追加メソッド
```cpp
virtual IRHIRaytracingPipelineState* CreateRaytracingPipelineState(
    const RHIRaytracingPipelineStateDesc& desc, const char* debugName = nullptr) = 0;
```

### IRHICommandContext追加メソッド
```cpp
virtual void SetRaytracingPipelineState(IRHIRaytracingPipelineState* pso) = 0;
virtual void DispatchRays(const RHIDispatchRaysDesc& desc) = 0;
```

## 設計ノート
- D3D12のState Object（D3D12_STATE_OBJECT_DESC）に対応
- DXILライブラリは複数指定可能（部分コンパイル対応）
- ローカルルートシグネチャはシェーダーエクスポート単位で関連付け
- ビルダーパターンはRHIMeshPipelineStateBuilderと同一スタイル
- maxPayloadSize/maxAttributeSizeはパフォーマンスに直結（小さいほど高速）
