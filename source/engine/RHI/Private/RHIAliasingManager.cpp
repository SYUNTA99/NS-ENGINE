/// @file RHIAliasingManager.cpp
/// @brief エイリアシングマネージャー実装
#include "RHIAliasingManager.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIAliasingManager
    //=========================================================================

    void RHIAliasingManager::RegisterResource(
        IRHIResource* resource, uint64 heapOffset, uint64 size, uint32 firstPass, uint32 lastPass)
    {
        // 同じヒープオフセット範囲を共有するグループを検索
        for (auto& group : m_groups)
        {
            // メモリ範囲が重なるかチェック
            uint64 groupEnd = group.GetHeapOffset() + group.GetSize();
            uint64 resourceEnd = heapOffset + size;

            bool overlaps = !(resourceEnd <= group.GetHeapOffset() || heapOffset >= groupEnd);

            if (overlaps)
            {
                group.AddResource(resource, firstPass, lastPass);
                m_memorySaved += size; // エイリアシングによる節約（概算）
                return;
            }
        }

        // 新しいグループを作成
        m_groups.emplace_back(heapOffset, size);
        m_groups.back().AddResource(resource, firstPass, lastPass);
    }

    void RHIAliasingManager::Analyze()
    {
        // エイリアシンググループの最適化
        // - 重複するグループのマージ
        // - メモリ節約量の正確な計算
        m_memorySaved = 0;
        for (const auto& group : m_groups)
        {
            // 各グループのリソース数が2以上ならエイリアシングが発生
            // 正確な計算にはグループ内リソースの列挙が必要
            (void)group;
        }
    }

    void RHIAliasingManager::GenerateBarriersForPass(uint32 passIndex, RHIAliasingBarrierBatch& outBarriers) const
    {
        for (const auto& group : m_groups)
        {
            group.GenerateBarriers(passIndex, outBarriers);
        }
    }

    void RHIAliasingManager::Reset()
    {
        m_groups.clear();
        m_memorySaved = 0;
    }

} // namespace NS::RHI
