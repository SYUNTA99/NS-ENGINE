# 21-04: コマンドシグネチャ

## 概要

ExecuteIndirect用のコマンドシグネチャ、
引数バッファのレイアウトとコマンドタイプを定義、

## ファイル

- `Source/Engine/RHI/Public/RHICommandSignature.h`

## 依存

- 21-01-indirect-arguments.md (引数構造体
- 08-03-root-signature.md (IRHIRootSignature)

## 定義

```cpp
namespace NS::RHI
{

/// Indirect引数タイプ
enum class ERHIIndirectArgumentType : uint8
{
    Draw,               ///< DrawInstanced引数
    DrawIndexed,        ///< DrawIndexedInstanced引数
    Dispatch,           ///< Dispatch引数
    DispatchMesh,       ///< DispatchMesh引数
    DispatchRays,       ///< DispatchRays引数
    VertexBufferView,   ///< 頂点バッファビュー変更
    IndexBufferView,    ///< インデックスバッファビュー変更
    Constant,           ///< ルート定数
    ConstantBufferView, ///< CBV変更
    ShaderResourceView, ///< SRV変更
    UnorderedAccessView ///< UAV変更
};

/// Indirect引数記述
struct RHIIndirectArgument
{
    ERHIIndirectArgumentType type = ERHIIndirectArgumentType::Draw;

    union
    {
        struct
        {
            uint32 rootParameterIndex;
            uint32 destOffsetIn32BitValues;
            uint32 num32BitValues;
        } constant;

        struct
        {
            uint32 rootParameterIndex;
        } cbv;

        struct
        {
            uint32 rootParameterIndex;
        } srv;

        struct
        {
            uint32 rootParameterIndex;
        } uav;

        struct
        {
            uint32 slot;
        } vertexBuffer;
    };

    /// サイズ取得
    uint32 GetByteSize() const;
};

/// コマンドシグネチャ記述
struct RHICommandSignatureDesc
{
    const RHIIndirectArgument* arguments = nullptr;
    uint32 argumentCount = 0;
    uint32 byteStride = 0;          ///< 0=自動計算
    IRHIRootSignature* rootSignature = nullptr; ///< リソース変更を含む場合必要）。
    const char* debugName = nullptr;
};

/// コマンドシグネチャインターフェース
/// ExecuteIndirectで使用する引数バッファのフォーマットを定義
class IRHICommandSignature : public IRHIResource
{
public:
    virtual ~IRHICommandSignature() = default;

    /// 引数のバイトストライド取得
    virtual uint32 GetByteStride() const = 0;

    /// 引数数取得
    virtual uint32 GetArgumentCount() const = 0;

    /// 引数タイプ取得
    virtual ERHIIndirectArgumentType GetArgumentType(uint32 index) const = 0;
};

using RHICommandSignatureRef = TRefCountPtr<IRHICommandSignature>;

// ════════════════════════════════════════════════════════════════
// ビルダー
// ════════════════════════════════════════════════════════════════

class RHICommandSignatureBuilder
{
public:
    RHICommandSignatureBuilder& AddDraw()
    {
        m_arguments.push_back({ ERHIIndirectArgumentType::Draw });
        return *this;
    }

    RHICommandSignatureBuilder& AddDrawIndexed()
    {
        m_arguments.push_back({ ERHIIndirectArgumentType::DrawIndexed });
        return *this;
    }

    RHICommandSignatureBuilder& AddDispatch()
    {
        m_arguments.push_back({ ERHIIndirectArgumentType::Dispatch });
        return *this;
    }

    RHICommandSignatureBuilder& AddDispatchMesh()
    {
        m_arguments.push_back({ ERHIIndirectArgumentType::DispatchMesh });
        return *this;
    }

    RHICommandSignatureBuilder& AddConstant(uint32 rootParameterIndex,
                                            uint32 destOffset,
                                            uint32 numValues)
    {
        RHIIndirectArgument arg{ ERHIIndirectArgumentType::Constant };
        arg.constant.rootParameterIndex = rootParameterIndex;
        arg.constant.destOffsetIn32BitValues = destOffset;
        arg.constant.num32BitValues = numValues;
        m_arguments.push_back(arg);
        return *this;
    }

    RHICommandSignatureBuilder& AddVertexBufferView(uint32 slot)
    {
        RHIIndirectArgument arg{ ERHIIndirectArgumentType::VertexBufferView };
        arg.vertexBuffer.slot = slot;
        m_arguments.push_back(arg);
        return *this;
    }

    RHICommandSignatureBuilder& AddCBV(uint32 rootParameterIndex)
    {
        RHIIndirectArgument arg{ ERHIIndirectArgumentType::ConstantBufferView };
        arg.cbv.rootParameterIndex = rootParameterIndex;
        m_arguments.push_back(arg);
        return *this;
    }

    RHICommandSignatureBuilder& SetRootSignature(IRHIRootSignature* rootSig)
    {
        m_rootSignature = rootSig;
        return *this;
    }

    RHICommandSignatureBuilder& SetDebugName(const char* name)
    {
        m_debugName = name;
        return *this;
    }

    RHICommandSignatureDesc Build() const
    {
        return {
            m_arguments.data(),
            static_cast<uint32>(m_arguments.size()),
            0,
            m_rootSignature,
            m_debugName
        };
    }

private:
    std::vector<RHIIndirectArgument> m_arguments;
    IRHIRootSignature* m_rootSignature = nullptr;
    const char* m_debugName = nullptr;
};

// ════════════════════════════════════════════════════════════════
// 標準コマンドシグネチャ
// ════════════════════════════════════════════════════════════════

/// 標準のコマンドシグネチャを取得
namespace RHIStandardCommandSignatures
{
    /// 単純なDrawIndexed用
    IRHICommandSignature* GetDrawIndexed(IRHIDevice* device);

    /// 単純なDispatch用
    IRHICommandSignature* GetDispatch(IRHIDevice* device);

    /// 単純なDispatchMesh用
    IRHICommandSignature* GetDispatchMesh(IRHIDevice* device);
}

} // namespace NS::RHI
```

## プラットフォーム実装

| API | 実装|
|-----|------|
| D3D12 | ID3D12CommandSignature |
| Vulkan | 直接対応なし（複数vkCmdDraw*Indirectで模倣（|
| Metal | ICB (Indirect Command Buffer) |

## 検証

- [ ] 各引数タイプのサイズ計算
- [ ] ルートシグネチャ要件の検証
- [ ] ビルダーパターンの動作
