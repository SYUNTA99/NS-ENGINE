# 22-03: メッシュPSO

## 概要

メッシュシェーダーパイプラインステートオブジェクト、
増幅シェーダー→メッシュシェーダー→ピクセルシェーダーのパイプライン、

## ファイル

- `Source/Engine/RHI/Public/RHIMeshPipelineState.h`

## 依存

- 22-01-mesh-shader.md (IRHIMeshShader)
- 22-02-amplification-shader.md (IRHIAmplificationShader)
- 07-05-graphics-pso.md (Graphics PSO基盤)

## 定義

```cpp
namespace NS::RHI
{

/// メッシュパイプラインステート記述
struct RHIMeshPipelineStateDesc
{
    // シェーダー
    IRHIAmplificationShader* amplificationShader = nullptr;  ///< オプション
    IRHIMeshShader* meshShader = nullptr;                    ///< 必要。
    /// @note pixelShaderのGetFrequency()がPixelであることをデバッグビルドで検証
    IRHIShader* pixelShader = nullptr;                  ///< 必要。

    // ルートシグネチャ
    IRHIRootSignature* rootSignature = nullptr;

    // レンダーステート
    RHIBlendStateDesc blendState;
    RHIRasterizerStateDesc rasterizerState;
    RHIDepthStencilStateDesc depthStencilState;

    // レンダーターゲットフォーマット
    uint32 numRenderTargets = 1;
    ERHIPixelFormat rtvFormats[8] = { ERHIPixelFormat::RGBA8_UNORM };
    ERHIPixelFormat dsvFormat = ERHIPixelFormat::D32_FLOAT;
    uint32 sampleCount = 1;

    // デバッグ
    const char* debugName = nullptr;
};

/// メッシュパイプラインステートインターフェース
class IRHIMeshPipelineState : public IRHIResource
{
public:
    virtual ~IRHIMeshPipelineState() = default;

    /// 増幅シェーダー取得
    virtual IRHIAmplificationShader* GetAmplificationShader() const = 0;

    /// メッシュシェーダー取得
    virtual IRHIMeshShader* GetMeshShader() const = 0;

    /// ピクセルシェーダー取得
    virtual IRHIShader* GetPixelShader() const = 0;

    /// ルートシグネチャ取得
    virtual IRHIRootSignature* GetRootSignature() const = 0;
};

using RHIMeshPipelineStateRef = TRefCountPtr<IRHIMeshPipelineState>;

// ════════════════════════════════════════════════════════════════
// ビルダー
// ════════════════════════════════════════════════════════════════

class RHIMeshPipelineStateBuilder
{
public:
    RHIMeshPipelineStateBuilder& SetAmplificationShader(IRHIAmplificationShader* shader)
    {
        m_desc.amplificationShader = shader;
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetMeshShader(IRHIMeshShader* shader)
    {
        m_desc.meshShader = shader;
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetPixelShader(IRHIShader* shader)
    {
        m_desc.pixelShader = shader;
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetRootSignature(IRHIRootSignature* rootSig)
    {
        m_desc.rootSignature = rootSig;
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetBlendState(const RHIBlendStateDesc& state)
    {
        m_desc.blendState = state;
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetRasterizerState(const RHIRasterizerStateDesc& state)
    {
        m_desc.rasterizerState = state;
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetDepthStencilState(const RHIDepthStencilStateDesc& state)
    {
        m_desc.depthStencilState = state;
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetRenderTargetFormat(uint32 index, ERHIPixelFormat format)
    {
        m_desc.rtvFormats[index] = format;
        m_desc.numRenderTargets = std::max(m_desc.numRenderTargets, index + 1);
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetDepthStencilFormat(ERHIPixelFormat format)
    {
        m_desc.dsvFormat = format;
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetSampleCount(uint32 count)
    {
        m_desc.sampleCount = count;
        return *this;
    }

    RHIMeshPipelineStateBuilder& SetDebugName(const char* name)
    {
        m_desc.debugName = name;
        return *this;
    }

    const RHIMeshPipelineStateDesc& Build() const { return m_desc; }

private:
    RHIMeshPipelineStateDesc m_desc;
};

// ════════════════════════════════════════════════════════════════
// 標準メッシュレットパイプライン
// ════════════════════════════════════════════════════════════════

/// メッシュレット描画用標準パイプライン設定
namespace RHIMeshletPipelinePresets
{
    /// 不透明メッシュレット描画
    RHIMeshPipelineStateDesc CreateOpaque(
        IRHIMeshShader* meshShader,
        IRHIShader* pixelShader,
        IRHIRootSignature* rootSig);

    /// LOD選択付きメッシュレット描画
    RHIMeshPipelineStateDesc CreateWithLODSelection(
        IRHIAmplificationShader* lodSelectShader,
        IRHIMeshShader* meshShader,
        IRHIShader* pixelShader,
        IRHIRootSignature* rootSig);

    /// GPUカリング付きメッシュレット描画
    RHIMeshPipelineStateDesc CreateWithGPUCulling(
        IRHIAmplificationShader* cullingShader,
        IRHIMeshShader* meshShader,
        IRHIShader* pixelShader,
        IRHIRootSignature* rootSig);
}

} // namespace NS::RHI
```

## 使用例

```cpp
// メッシュシェーダーPSO構築
auto meshPSO = device->CreateMeshPipelineState(
    RHIMeshPipelineStateBuilder()
        .SetAmplificationShader(lodSelectAS)
        .SetMeshShader(meshletMS)
        .SetPixelShader(gbufferPS)
        .SetRootSignature(meshletRootSig)
        .SetRenderTargetFormat(0, ERHIPixelFormat::RGBA8_UNORM)
        .SetRenderTargetFormat(1, ERHIPixelFormat::RGB10A2_UNORM)
        .SetDepthStencilFormat(ERHIPixelFormat::D32_FLOAT)
        .SetDebugName("MeshletGBuffer")
        .Build()
);

// 描画
context->SetMeshPipelineState(meshPSO);
context->SetGraphicsRootSignature(meshletRootSig);
context->DispatchMesh(meshletCount, 1, 1);
```

## プラットフォーム対応

| API | 対応状況|
|-----|---------|
| D3D12 | SM 6.5+、D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS |
| Vulkan | VK_EXT_mesh_shader |
| Metal | MTLMeshRenderPipelineDescriptor |

## 検証

- [ ] AS + MS + PS パイプライン構築
- [ ] MS + PS のみ（ASなし）構築
- [ ] 各レンダーステートの適用
- [ ] ルートシグネチャ互換性
