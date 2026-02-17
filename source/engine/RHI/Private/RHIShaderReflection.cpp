/// @file RHIShaderReflection.cpp
/// @brief シェーダーリフレクション実装
#include "RHIShaderReflection.h"
#include "IRHIShader.h"

#include <algorithm>
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIInputSignature
    //=========================================================================

    const RHIShaderParameter* RHIInputSignature::FindBySemantic(const char* semanticName, uint32 semanticIndex) const
    {
        for (const auto& param : parameters)
        {
            if (std::strcmp(param.semanticName, semanticName) == 0 && param.semanticIndex == semanticIndex) {
                return &param;
}
        }
        return nullptr;
    }

    uint32 RHIInputSignature::CalculateTotalSize() const
    {
        uint32 totalSize = 0;
        for (const auto& param : parameters)
        {
            // マスクからコンポーネント数を算出
            uint32 componentCount = 0;
            uint8 mask = param.mask;
            while (mask != 0u)
            {
                componentCount += (mask & 1);
                mask >>= 1;
            }

            // コンポーネントタイプのサイズ
            uint32 const componentSize = 4; // 32bit（デフォルト）
            totalSize += componentCount * componentSize;
        }
        return totalSize;
    }

    //=========================================================================
    // RHIOutputSignature
    //=========================================================================

    uint32 RHIOutputSignature::GetRenderTargetCount() const
    {
        uint32 count = 0;
        for (const auto& param : parameters)
        {
            if (param.systemValue == RHIShaderParameter::SystemValue::Target) {
                ++count;
}
        }
        return count;
    }

    bool RHIOutputSignature::HasDepthOutput() const
    {
        for (const auto& param : parameters)
        {
            if (param.systemValue == RHIShaderParameter::SystemValue::Depth) {
                return true;
}
        }
        return false;
    }

    //=========================================================================
    // リフレクション作成関数
    //=========================================================================

    IRHIShaderReflection* CreateShaderReflection(const RHIShaderBytecode& bytecode)
    {
        (void)bytecode;
        // シェーダーリフレクションの実装はバックエンド依存
        // D3D12: D3DReflect() を使用
        // Vulkan: SPIRV-Cross を使用
        return nullptr;
    }

    IRHIShaderReflection* CreateShaderReflection(IRHIShader* shader)
    {
        (void)shader;
        // shader->GetBytecode() からリフレクション取得
        return nullptr;
    }

    //=========================================================================
    // RHIBindingLayoutBuilder
    //=========================================================================

    void RHIBindingLayoutBuilder::AddShader(IRHIShaderReflection* reflection)
    {
        if (reflection != nullptr) {
            m_reflections.push_back(reflection);
}
    }

    void RHIBindingLayoutBuilder::AddShader(const RHIShaderBytecode& bytecode)
    {
        IRHIShaderReflection* reflection = CreateShaderReflection(bytecode);
        if (reflection != nullptr) {
            m_reflections.push_back(reflection);
}
    }

    bool RHIBindingLayoutBuilder::Build()
    {
        m_resourceBindings.clear();
        m_constantBuffers.clear();

        for (auto* reflection : m_reflections)
        {
            // リソースバインディングを収集
            uint32 const bindingCount = reflection->GetResourceBindingCount();
            for (uint32 i = 0; i < bindingCount; ++i)
            {
                RHIShaderResourceBinding binding;
                if (reflection->GetResourceBinding(i, binding))
                {
                    // 重複チェック（同じスペース+バインドポイント+タイプなら既存を再利用）
                    bool found = false;
                    for (auto& existing : m_resourceBindings)
                    {
                        if (existing.space == binding.space && existing.bindPoint == binding.bindPoint &&
                            existing.type == binding.type)
                        {
                            // bindCountを最大に更新
                            existing.bindCount = std::max(binding.bindCount, existing.bindCount);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        m_resourceBindings.push_back(binding);
}
                }
            }

            // 定数バッファを収集
            uint32 const cbCount = reflection->GetConstantBufferCount();
            for (uint32 i = 0; i < cbCount; ++i)
            {
                RHIShaderConstantBuffer cb;
                if (reflection->GetConstantBuffer(i, cb))
                {
                    bool found = false;
                    for (auto& existing : m_constantBuffers)
                    {
                        if (existing.space == cb.space && existing.bindPoint == cb.bindPoint)
                        {
                            // サイズを最大に更新
                            if (cb.size > existing.size) {
                                existing = cb;
}
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        m_constantBuffers.push_back(cb);
}
                }
            }
        }

        return !m_resourceBindings.empty() || !m_constantBuffers.empty();
    }

    uint32 RHIBindingLayoutBuilder::GetMaxRegisterSpace() const
    {
        uint32 maxSpace = 0;
        for (const auto& binding : m_resourceBindings)
        {
            maxSpace = std::max(binding.space, maxSpace);
        }
        for (const auto& cb : m_constantBuffers)
        {
            maxSpace = std::max(cb.space, maxSpace);
        }
        return maxSpace;
    }

    bool RHIBindingLayoutBuilder::RecommendBindless(uint32 threshold) const
    {
        return static_cast<uint32>(m_resourceBindings.size()) >= threshold;
    }

} // namespace NS::RHI
