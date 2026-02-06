/// @file LowLevelMemTracker.cpp
/// @brief Low Level Memory Tracker 実装

#include "HAL/LowLevelMemTracker.h"

#if ENABLE_LOW_LEVEL_MEM_TRACKER

#include <cstring>

namespace NS
{
    // =========================================================================
    // 内部グローバル変数
    // =========================================================================

    namespace Private
    {
        bool g_llmRegistrationPhase = true;
        uint32 g_llmProjectTagCount = 0;
        LLMCustomTagInfo g_llmProjectTags[kLLMMaxProjectTags] = {};
    } // namespace Private

    // =========================================================================
    // TLSタグスタック
    // =========================================================================

    static thread_local LLMTag s_tagStack[LowLevelMemTracker::kMaxTagStackDepth];
    static thread_local int32 s_tagStackTop = 0;

    static thread_local LLMTagSet s_tagSetStack[LowLevelMemTracker::kMaxTagStackDepth];
    static thread_local int32 s_tagSetStackTop = 0;

    // =========================================================================
    // 組み込みタグ名
    // =========================================================================

    static const TCHAR* GetBuiltinTagName(LLMTag tag)
    {
        switch (tag)
        {
        case LLMTag::Untagged:
            return TEXT("Untagged");
        case LLMTag::Paused:
            return TEXT("Paused");
        case LLMTag::Total:
            return TEXT("Total");
        case LLMTag::Untracked:
            return TEXT("Untracked");
        case LLMTag::TrackedTotal:
            return TEXT("TrackedTotal");
        case LLMTag::EngineMisc:
            return TEXT("EngineMisc");
        case LLMTag::Malloc:
            return TEXT("Malloc");
        case LLMTag::Containers:
            return TEXT("Containers");
        case LLMTag::Textures:
            return TEXT("Textures");
        case LLMTag::RenderTargets:
            return TEXT("RenderTargets");
        case LLMTag::Shaders:
            return TEXT("Shaders");
        case LLMTag::Meshes:
            return TEXT("Meshes");
        case LLMTag::Particles:
            return TEXT("Particles");
        case LLMTag::RHIMisc:
            return TEXT("RHIMisc");
        case LLMTag::Audio:
            return TEXT("Audio");
        case LLMTag::Physics:
            return TEXT("Physics");
        case LLMTag::UI:
            return TEXT("UI");
        case LLMTag::Networking:
            return TEXT("Networking");
        case LLMTag::Animation:
            return TEXT("Animation");
        case LLMTag::AI:
            return TEXT("AI");
        case LLMTag::Scripting:
            return TEXT("Scripting");
        case LLMTag::World:
            return TEXT("World");
        case LLMTag::Actors:
            return TEXT("Actors");
        default:
            return nullptr;
        }
    }

    // =========================================================================
    // カスタムタグAPI
    // =========================================================================

    LLMTag RegisterLLMCustomTag(const TCHAR* name, const TCHAR* statGroup, LLMTag parentTag)
    {
        if (!Private::g_llmRegistrationPhase)
        {
            return LLMTag::Untagged;
        }

        if (Private::g_llmProjectTagCount >= kLLMMaxProjectTags)
        {
            return LLMTag::Untagged;
        }

        uint32 index = Private::g_llmProjectTagCount++;
        Private::g_llmProjectTags[index].name = name;
        Private::g_llmProjectTags[index].statGroup = statGroup;
        Private::g_llmProjectTags[index].parentTag = parentTag;

        return static_cast<LLMTag>(kLLMProjectTagStart + index);
    }

    void FinalizeTagRegistration()
    {
        Private::g_llmRegistrationPhase = false;
    }

    bool IsInRegistrationPhase()
    {
        return Private::g_llmRegistrationPhase;
    }

    const TCHAR* GetLLMTagName(LLMTag tag)
    {
        uint8 tagValue = static_cast<uint8>(tag);

        // 組み込みタグ
        const TCHAR* builtinName = GetBuiltinTagName(tag);
        if (builtinName)
        {
            return builtinName;
        }

        // プロジェクトタグ
        if (tagValue >= kLLMProjectTagStart && tagValue <= kLLMProjectTagEnd)
        {
            uint32 index = tagValue - kLLMProjectTagStart;
            if (index < Private::g_llmProjectTagCount)
            {
                return Private::g_llmProjectTags[index].name;
            }
        }

        return TEXT("Unknown");
    }

    bool IsValidLLMTag(LLMTag tag)
    {
        uint8 tagValue = static_cast<uint8>(tag);

        // 組み込みタグ
        if (tagValue <= static_cast<uint8>(LLMTag::GenericTagEnd))
        {
            return true;
        }

        // プロジェクトタグ
        if (tagValue >= kLLMProjectTagStart)
        {
            uint32 index = tagValue - kLLMProjectTagStart;
            return index < Private::g_llmProjectTagCount;
        }

        return false;
    }

    uint32 GetLLMCustomTagCount()
    {
        return Private::g_llmProjectTagCount;
    }

    // =========================================================================
    // LowLevelMemTracker
    // =========================================================================

    LowLevelMemTracker& LowLevelMemTracker::Get()
    {
        static LowLevelMemTracker instance;
        return instance;
    }

    LowLevelMemTracker::LowLevelMemTracker() : m_enabled(true)
    {
        std::memset(m_tagData, 0, sizeof(m_tagData));
    }

    void LowLevelMemTracker::PushTag(LLMTag tag)
    {
        if (!m_enabled)
        {
            return;
        }

        if (s_tagStackTop < kMaxTagStackDepth)
        {
            s_tagStack[s_tagStackTop++] = tag;
        }
    }

    void LowLevelMemTracker::PopTag()
    {
        if (!m_enabled)
        {
            return;
        }

        if (s_tagStackTop > 0)
        {
            --s_tagStackTop;
        }
    }

    LLMTag LowLevelMemTracker::GetCurrentTag() const
    {
        if (s_tagStackTop > 0)
        {
            return s_tagStack[s_tagStackTop - 1];
        }
        return LLMTag::Untagged;
    }

    void LowLevelMemTracker::PushTagSet(LLMTag tag, LLMTagSet tagSet)
    {
        if (!m_enabled)
        {
            return;
        }

        PushTag(tag);

        if (s_tagSetStackTop < kMaxTagStackDepth)
        {
            s_tagSetStack[s_tagSetStackTop++] = tagSet;
        }
    }

    void LowLevelMemTracker::PopTagSet()
    {
        if (!m_enabled)
        {
            return;
        }

        PopTag();

        if (s_tagSetStackTop > 0)
        {
            --s_tagSetStackTop;
        }
    }

    void LowLevelMemTracker::TrackAllocation(LLMTag tag, int64 size)
    {
        if (!m_enabled || tag == LLMTag::Paused)
        {
            return;
        }

        uint8 tagIndex = static_cast<uint8>(tag);
        m_tagData[tagIndex].amount += size;
        m_tagData[tagIndex].totalAllocations++;

        if (m_tagData[tagIndex].amount > m_tagData[tagIndex].peak)
        {
            m_tagData[tagIndex].peak = m_tagData[tagIndex].amount;
        }
    }

    void LowLevelMemTracker::TrackFree(LLMTag tag, int64 size)
    {
        if (!m_enabled || tag == LLMTag::Paused)
        {
            return;
        }

        uint8 tagIndex = static_cast<uint8>(tag);
        m_tagData[tagIndex].amount -= size;
    }

    void LowLevelMemTracker::OnLowLevelAlloc(
        LLMTracker tracker, void* ptr, int64 size, LLMTag tag, LLMAllocType allocType)
    {
        NS_UNUSED(tracker);
        NS_UNUSED(ptr);
        NS_UNUSED(allocType);

        TrackAllocation(tag, size);
    }

    void LowLevelMemTracker::OnLowLevelFree(LLMTracker tracker, void* ptr, LLMAllocType allocType)
    {
        NS_UNUSED(tracker);
        NS_UNUSED(ptr);
        NS_UNUSED(allocType);
        // サイズ情報がないため、実装は簡略化
    }

    void LowLevelMemTracker::OnLowLevelAllocMoved(LLMTracker tracker, void* destPtr, void* sourcePtr)
    {
        NS_UNUSED(tracker);
        NS_UNUSED(destPtr);
        NS_UNUSED(sourcePtr);
        // ポインタ移動は統計に影響しない
    }

    void LowLevelMemTracker::OnLowLevelChangeInMemoryUse(LLMTracker tracker, int64 deltaMemory, LLMTag tag)
    {
        NS_UNUSED(tracker);

        if (deltaMemory > 0)
        {
            TrackAllocation(tag, deltaMemory);
        }
        else
        {
            TrackFree(tag, -deltaMemory);
        }
    }

    int64 LowLevelMemTracker::GetTagAmount(LLMTag tag) const
    {
        uint8 tagIndex = static_cast<uint8>(tag);
        return m_tagData[tagIndex].amount;
    }

    uint32 LowLevelMemTracker::GetTagStats(LLMTagStats* outStats, uint32 maxCount) const
    {
        uint32 count = 0;

        for (uint32 i = 0; i < kLLMMaxTagCount && count < maxCount; ++i)
        {
            if (m_tagData[i].amount != 0 || m_tagData[i].peak != 0)
            {
                outStats[count].tag = static_cast<LLMTag>(i);
                outStats[count].amount = m_tagData[i].amount;
                outStats[count].peak = m_tagData[i].peak;
                ++count;
            }
        }

        return count;
    }

    void LowLevelMemTracker::DumpStats()
    {
        // 統計出力（OutputDevice実装後に完成）
    }

    void LowLevelMemTracker::ResetStats()
    {
        std::memset(m_tagData, 0, sizeof(m_tagData));
    }

} // namespace NS

#endif // ENABLE_LOW_LEVEL_MEM_TRACKER
