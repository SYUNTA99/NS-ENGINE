/// @file RHIBreadcrumbs.cpp
/// @brief Breadcrumbsシステム実装
#include "RHIBreadcrumbs.h"
#include "IRHICommandContext.h"
#include <cstdio>
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIBreadcrumbNode
    //=========================================================================

    uint32 RHIBreadcrumbNode::GetFullPath(char* outBuffer, uint32 bufferSize) const
    {
        if ((outBuffer == nullptr) || bufferSize == 0)
        {
            return 0;
        }

        // 親からのパスをスタックで構築
        const RHIBreadcrumbNode* stack[64];
        uint32 depth = 0;

        const RHIBreadcrumbNode* node = this;
        while ((node != nullptr) && depth < 64)
        {
            stack[depth++] = node;
            node = node->parent;
        }

        uint32 written = 0;
        for (uint32 i = depth; i > 0; --i)
        {
            const char* name = (stack[i - 1]->data != nullptr) ? stack[i - 1]->data->staticName : "?";
            if (written > 0 && written < bufferSize - 1)
            {
                outBuffer[written++] = '/';
            }

            auto const nameLen = static_cast<uint32>(std::strlen(name));
            uint32 const copyLen = (nameLen < bufferSize - written - 1) ? nameLen : (bufferSize - written - 1);
            std::memcpy(outBuffer + written, name, copyLen);
            written += copyLen;
        }

        outBuffer[written] = '\0';
        return written;
    }

    uint32 RHIBreadcrumbNode::WriteCrashData(char* outBuffer, uint32 bufferSize) const
    {
        if ((outBuffer == nullptr) || bufferSize == 0)
        {
            return 0;
        }

        char pathBuffer[256];
        GetFullPath(pathBuffer, sizeof(pathBuffer));

        int written = 0;
        if ((data != nullptr) && (data->sourceFile != nullptr))
        {
            written = std::snprintf(
                outBuffer, bufferSize, "[BC#%u] %s (%s:%u)", id, pathBuffer, data->sourceFile, data->sourceLine);
        }
        else
        {
            written = std::snprintf(outBuffer, bufferSize, "[BC#%u] %s", id, pathBuffer);
        }

        return (written > 0) ? static_cast<uint32>(written) : 0;
    }

    //=========================================================================
    // RHIBreadcrumbAllocator
    //=========================================================================

    bool RHIBreadcrumbAllocator::Initialize(uint32 maxNodes)
    {
        m_nodes = new RHIBreadcrumbNode[maxNodes];
        m_capacity = maxNodes;
        m_nextId = 0;
        return true;
    }

    void RHIBreadcrumbAllocator::Shutdown()
    {
        delete[] m_nodes;
        m_nodes = nullptr;
        m_capacity = 0;
        m_nextId = 0;
    }

    RHIBreadcrumbNode* RHIBreadcrumbAllocator::AllocateNode(RHIBreadcrumbNode* parent, const RHIBreadcrumbData& data)
    {
        if (m_nextId >= m_capacity)
        {
            return nullptr;
        }

        auto& node = m_nodes[m_nextId];
        node.id = m_nextId;
        node.parent = parent;
        node.data = &data;

        ++m_nextId;
        return &node;
    }

    void RHIBreadcrumbAllocator::Reset()
    {
        m_nextId = 0;
    }

    //=========================================================================
    // RHIBreadcrumbState
    //=========================================================================

    static thread_local RHIBreadcrumbState t_breadcrumbState;

    RHIBreadcrumbState& RHIBreadcrumbState::Get()
    {
        return t_breadcrumbState;
    }

    RHIBreadcrumbNode* RHIBreadcrumbState::GetCurrentNode() const
    {
        return (m_stackDepth > 0) ? m_nodeStack[m_stackDepth - 1] : nullptr;
    }

    void RHIBreadcrumbState::PushNode(RHIBreadcrumbNode* node)
    {
        if (m_stackDepth < kMaxStackDepth)
        {
            m_nodeStack[m_stackDepth++] = node;
        }
    }

    void RHIBreadcrumbState::PopNode()
    {
        if (m_stackDepth > 0)
        {
            --m_stackDepth;
        }
    }

    void RHIBreadcrumbState::DumpActiveBreadcrumbs()
    {
        auto& state = Get();
        char buffer[512];
        for (uint32 i = 0; i < state.m_stackDepth; ++i)
        {
            state.m_nodeStack[i]->WriteCrashData(buffer, sizeof(buffer));
            // ログ出力（プラットフォーム依存）
        }
    }

    //=========================================================================
    // RHIBreadcrumbScope
    //=========================================================================

    RHIBreadcrumbScope::RHIBreadcrumbScope(IRHICommandContext* context,
                                           RHIBreadcrumbAllocator* allocator,
                                           const char* name,
                                           const char* sourceFile,
                                           uint32 sourceLine)
        : m_context(context)
    {
        if ((context == nullptr) || (allocator == nullptr))
        {
            return;
        }

        static thread_local RHIBreadcrumbData data;
        data.staticName = name;
        data.sourceFile = sourceFile;
        data.sourceLine = sourceLine;

        auto& state = RHIBreadcrumbState::Get();
        m_node = allocator->AllocateNode(state.GetCurrentNode(), data);
        if (m_node != nullptr)
        {
            state.PushNode(m_node);
        }
    }

    RHIBreadcrumbScope::~RHIBreadcrumbScope()
    {
        if (m_node != nullptr)
        {
            RHIBreadcrumbState::Get().PopNode();
        }
    }

} // namespace NS::RHI
