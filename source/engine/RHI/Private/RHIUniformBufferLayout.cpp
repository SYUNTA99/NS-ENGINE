/// @file RHIUniformBufferLayout.cpp
/// @brief UniformBufferレイアウト・ビルダー実装
#include "RHIUniformBufferLayout.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIUniformBufferLayout
    //=========================================================================

    const RHIUniformElement* RHIUniformBufferLayout::FindElement(const char* name) const
    {
        if (name == nullptr) {
            return nullptr;
}

        for (const auto& elem : m_elements)
        {
            if ((elem.name != nullptr) && std::strcmp(elem.name, name) == 0) {
                return &elem;
}
        }
        return nullptr;
    }

    //=========================================================================
    // RHIUniformBufferLayoutBuilder
    //=========================================================================

    RHIUniformBufferLayoutBuilder& RHIUniformBufferLayoutBuilder::AddElement(const char* name,
                                                                             ERHIUniformType type,
                                                                             uint32 size,
                                                                             uint32 alignment)
    {
        m_currentOffset = AlignOffset(m_currentOffset, alignment);

        // HLSLルール: 16バイト境界をまたぐ場合はパディング
        uint32 const boundary16 = (m_currentOffset / 16 + 1) * 16;
        if (m_currentOffset + size > boundary16 && m_currentOffset % 16 != 0)
        {
            m_currentOffset = boundary16;
        }

        RHIUniformElement element;
        element.name = name;
        element.type = type;
        element.offset = m_currentOffset;
        element.size = size;
        element.arrayCount = 1;
        element.arrayStride = 0;

        m_elements.push_back(element);
        m_currentOffset += size;

        return *this;
    }

    RHIUniformBufferLayoutBuilder& RHIUniformBufferLayoutBuilder::AddArrayElement(
        const char* name, ERHIUniformType type, uint32 elementSize, uint32 alignment, uint32 count)
    {
        m_currentOffset = AlignOffset(m_currentOffset, alignment);

        // HLSL配列: 各要素は16バイトアライン
        uint32 const stride = AlignOffset(elementSize, 16);

        RHIUniformElement element;
        element.name = name;
        element.type = type;
        element.offset = m_currentOffset;
        element.size = elementSize;
        element.arrayCount = count;
        element.arrayStride = stride;

        m_elements.push_back(element);
        m_currentOffset += stride * count;

        return *this;
    }

    RHIUniformBufferLayoutRef RHIUniformBufferLayoutBuilder::Build()
    {
        auto layout = std::make_shared<RHIUniformBufferLayout>();
        layout->m_elements = std::move(m_elements);
        layout->m_size = AlignOffset(m_currentOffset, 16); // 16バイトアライン
        layout->m_debugName = m_debugName;

        // FNV-1aハッシュ
        uint64 hash = 14695981039346656037ULL;
        for (const auto& elem : layout->m_elements)
        {
            // 名前のハッシュ
            if (elem.name != nullptr)
            {
                const char* p = elem.name;
                while (*p != 0)
                {
                    hash ^= static_cast<uint64>(*p++);
                    hash *= 1099511628211ULL;
                }
            }
            // オフセットとサイズ
            hash ^= static_cast<uint64>(elem.offset);
            hash *= 1099511628211ULL;
            hash ^= static_cast<uint64>(elem.size);
            hash *= 1099511628211ULL;
        }
        layout->m_hash = hash;

        // ビルダーリセット
        m_elements.clear();
        m_currentOffset = 0;
        m_debugName = nullptr;

        return layout;
    }

} // namespace NS::RHI
