/// @file RHIAliasingManager.h
/// @brief エイリアシンググループ・マネージャー
/// @details 同じ物理メモリを共有するリソース間の遷移を管理し、Transientリソースの再利用を可能にする。
/// @see 23-04-aliasing-barrier.md
#pragma once

#include "RHIBarrier.h"
#include "RHIMacros.h"
#include "RHITypes.h"

#include <vector>

namespace NS::RHI
{
    // 前方宣言
    class IRHIResource;

    //=========================================================================
    // RHIAliasingBarrierBatch (23-04)
    //=========================================================================

    /// エイリアシングバリアバッチ
    class RHI_API RHIAliasingBarrierBatch
    {
    public:
        /// バリア追加
        void Add(IRHIResource* before, IRHIResource* after)
        {
            m_barriers.push_back(RHIAliasingBarrier::Create(before, after));
        }

        /// 破棄（リソース使用終了）
        void AddDiscard(IRHIResource* resource) { m_barriers.push_back(RHIAliasingBarrier::Create(resource, nullptr)); }

        /// 取得バリア追加（リソース使用開始）
        void AddAcquire(IRHIResource* resource) { m_barriers.push_back(RHIAliasingBarrier::Create(nullptr, resource)); }

        /// クリア
        void Clear() { m_barriers.clear(); }

        /// 空かどうか
        bool IsEmpty() const { return m_barriers.empty(); }

        /// バリア数
        uint32 GetCount() const { return static_cast<uint32>(m_barriers.size()); }

        /// データ取得
        const RHIAliasingBarrier* GetData() const { return m_barriers.data(); }

    private:
        std::vector<RHIAliasingBarrier> m_barriers;
    };

    //=========================================================================
    // RHIAliasingGroup (23-04)
    //=========================================================================

    /// エイリアシンググループ
    /// 同じ物理メモリを共有するリソースの集合
    class RHI_API RHIAliasingGroup
    {
    public:
        explicit RHIAliasingGroup(uint64 heapOffset, uint64 size) : m_heapOffset(heapOffset), m_size(size) {}

        /// リソースをグループに追加
        void AddResource(IRHIResource* resource, uint32 firstPass, uint32 lastPass)
        {
            m_resources.push_back({resource, firstPass, lastPass});
        }

        /// 指定パスで必要なバリアを生成
        void GenerateBarriers(uint32 passIndex, RHIAliasingBarrierBatch& outBarriers) const
        {
            if (passIndex == 0)
                return; // 最初のパスではエイリアシング不要

            std::vector<IRHIResource*> resourcesBefore;
            IRHIResource* resourceAfter = nullptr;

            for (const auto& entry : m_resources)
            {
                if (entry.lastPass == passIndex - 1)
                {
                    resourcesBefore.push_back(entry.resource);
                }
                if (entry.firstPass == passIndex)
                {
                    resourceAfter = entry.resource;
                }
            }

            if (resourceAfter)
            {
                if (resourcesBefore.empty())
                {
                    outBarriers.AddAcquire(resourceAfter);
                }
                else
                {
                    for (IRHIResource* before : resourcesBefore)
                    {
                        outBarriers.Add(before, resourceAfter);
                    }
                }
            }
        }

        /// メモリ領域取得
        uint64 GetHeapOffset() const { return m_heapOffset; }
        uint64 GetSize() const { return m_size; }

    private:
        struct ResourceEntry
        {
            IRHIResource* resource;
            uint32 firstPass;
            uint32 lastPass;
        };

        uint64 m_heapOffset;
        uint64 m_size;
        std::vector<ResourceEntry> m_resources;
    };

    //=========================================================================
    // RHIAliasingManager (23-04)
    //=========================================================================

    /// エイリアシングマネージャー
    /// フレームグラフと連携してエイリアシングを最適化
    class RHI_API RHIAliasingManager
    {
    public:
        /// リソースを登録
        void RegisterResource(
            IRHIResource* resource, uint64 heapOffset, uint64 size, uint32 firstPass, uint32 lastPass);

        /// エイリアシング解析を実行
        void Analyze();

        /// 指定パスのバリアを生成
        void GenerateBarriersForPass(uint32 passIndex, RHIAliasingBarrierBatch& outBarriers) const;

        /// エイリアシングで節約されたメモリ量
        uint64 GetMemorySaved() const { return m_memorySaved; }

        /// リセット
        void Reset();

    private:
        std::vector<RHIAliasingGroup> m_groups;
        uint64 m_memorySaved = 0;
    };

} // namespace NS::RHI
