/// @file RHIMultiGPU.h
/// @brief マルチGPUサポート
/// @details ノード管理、クロスノードリソース共有、AFR/SFRレンダリング支援を提供。
/// @see 19-04-multi-gpu.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

namespace NS::RHI
{
    // 前方宣言
    class IRHIDevice;
    class IRHIFence;

    //=========================================================================
    // ERHIGPUNode (19-04)
    //=========================================================================

    /// GPUノードインデックス型
    using ERHIGPUNode = uint32;

    /// 無効ノード
    constexpr ERHIGPUNode kInvalidGPUNode = ~0u;

    /// 最大サポートGPUノード数
    constexpr uint32 kMaxGPUNodes = 4;

    //=========================================================================
    // RHINodeAffinityMask (19-04)
    //=========================================================================

    /// ノードアフィニティマスク
    /// どのGPUノードにリソースを配置/操作を実行するかを指定
    struct RHI_API RHINodeAffinityMask
    {
        uint32 mask = 1;

        constexpr RHINodeAffinityMask() = default;
        explicit constexpr RHINodeAffinityMask(uint32 m) : mask(m) {}

        /// 全ノード
        static constexpr RHINodeAffinityMask All(uint32 nodeCount)
        {
            return RHINodeAffinityMask((1u << nodeCount) - 1);
        }

        /// 単一ノード
        static constexpr RHINodeAffinityMask Single(ERHIGPUNode node) { return RHINodeAffinityMask(1u << node); }

        /// ノード0のみ（デフォルト）
        static constexpr RHINodeAffinityMask Node0() { return RHINodeAffinityMask(1); }

        /// 指定ノードを含むか
        constexpr bool Contains(ERHIGPUNode node) const { return (mask & (1u << node)) != 0; }

        /// 有効ノード数
        uint32 CountNodes() const
        {
            uint32 count = 0;
            uint32 m = mask;
            while (m)
            {
                count += (m & 1);
                m >>= 1;
            }
            return count;
        }

        /// 単一ノードか
        bool IsSingleNode() const { return CountNodes() == 1; }

        /// 最初のノードインデックス取得
        ERHIGPUNode GetFirstNode() const
        {
            for (uint32 i = 0; i < kMaxGPUNodes; ++i)
            {
                if (Contains(i))
                    return i;
            }
            return kInvalidGPUNode;
        }

        /// GPUMaskとの変換
        GPUMask ToGPUMask() const { return GPUMask(mask); }
        static RHINodeAffinityMask FromGPUMask(GPUMask gpuMask) { return RHINodeAffinityMask(gpuMask.mask); }

        constexpr RHINodeAffinityMask operator|(RHINodeAffinityMask other) const
        {
            return RHINodeAffinityMask(mask | other.mask);
        }
        constexpr RHINodeAffinityMask operator&(RHINodeAffinityMask other) const
        {
            return RHINodeAffinityMask(mask & other.mask);
        }
        constexpr bool operator==(RHINodeAffinityMask other) const { return mask == other.mask; }
        constexpr bool operator!=(RHINodeAffinityMask other) const { return mask != other.mask; }
    };

    //=========================================================================
    // RHIMultiGPUCapabilities (19-04)
    //=========================================================================

    /// マルチGPUケイパビリティ
    struct RHI_API RHIMultiGPUCapabilities
    {
        /// GPUノード数
        uint32 nodeCount = 1;

        /// ノード間リソース共有サポート
        bool crossNodeSharing = false;

        /// ノード間コピーサポート
        bool crossNodeCopy = false;

        /// ノード間テクスチャ共有サポート
        bool crossNodeTextureSharing = false;

        /// ノード間アトミック操作サポート
        bool crossNodeAtomics = false;

        /// リンクドアダプターか（同一物理GPU上の複数ノード）
        bool isLinkedAdapter = false;

        /// マルチGPU有効か
        bool IsMultiGPU() const { return nodeCount > 1; }

        /// シングルGPUか
        bool IsSingleGPU() const { return nodeCount <= 1; }
    };

    //=========================================================================
    // RHICrossNodeResourceDesc (19-04)
    //=========================================================================

    /// クロスノードリソース記述
    struct RHI_API RHICrossNodeResourceDesc
    {
        /// ソースGPUノード
        ERHIGPUNode sourceNode = 0;

        /// デスティネーションGPUノード
        ERHIGPUNode destNode = 0;

        /// 共有対象リソース
        IRHIResource* resource = nullptr;

        /// 共有リソースのCreationNodeMask
        RHINodeAffinityMask creationNodeMask;

        /// 共有リソースのVisibleNodeMask
        RHINodeAffinityMask visibleNodeMask;
    };

    //=========================================================================
    // RHICrossNodeCopyDesc (19-04)
    //=========================================================================

    /// クロスノードコピー記述
    struct RHI_API RHICrossNodeCopyDesc
    {
        /// ソースリソース
        IRHIResource* sourceResource = nullptr;

        /// デスティネーションリソース
        IRHIResource* destResource = nullptr;

        /// ソースGPUノード
        ERHIGPUNode sourceNode = 0;

        /// デスティネーションGPUノード
        ERHIGPUNode destNode = 0;
    };

    //=========================================================================
    // RHICrossNodeFenceSync (19-04)
    //=========================================================================

    /// クロスノードフェンス同期
    struct RHI_API RHICrossNodeFenceSync
    {
        /// 同期フェンス
        IRHIFence* fence = nullptr;

        /// シグナルするノード
        ERHIGPUNode signalNode = 0;

        /// 待機するノード
        ERHIGPUNode waitNode = 0;

        /// フェンス値
        uint64 fenceValue = 0;
    };

    //=========================================================================
    // IRHIMultiGPUDevice (19-04)
    //=========================================================================

    /// マルチGPUデバイスインターフェース
    /// ノード管理とクロスノード操作を提供
    class RHI_API IRHIMultiGPUDevice
    {
    public:
        virtual ~IRHIMultiGPUDevice() = default;

        //=====================================================================
        // ノード管理
        //=====================================================================

        /// ノード数取得
        virtual uint32 GetNodeCount() const = 0;

        /// 指定ノードのデバイス取得
        virtual IRHIDevice* GetNodeDevice(ERHIGPUNode node) const = 0;

        /// 全ノードマスク取得
        RHINodeAffinityMask GetAllNodesMask() const { return RHINodeAffinityMask::All(GetNodeCount()); }

        //=====================================================================
        // クロスノードリソース
        //=====================================================================

        /// クロスノード共有リソース作成
        virtual IRHIResource* CreateCrossNodeResource(const RHICrossNodeResourceDesc& desc) = 0;

        /// ノード間コピー
        virtual void CrossNodeCopy(const RHICrossNodeCopyDesc& desc) = 0;

        //=====================================================================
        // ノード間同期
        //=====================================================================

        /// ノード間フェンスシグナル
        virtual void SignalCrossNode(const RHICrossNodeFenceSync& sync) = 0;

        /// ノード間フェンス待機
        virtual void WaitCrossNode(const RHICrossNodeFenceSync& sync) = 0;
    };

    //=========================================================================
    // RHIAlternateFrameRenderer (19-04)
    //=========================================================================

    /// AFR（Alternate Frame Rendering）ヘルパー
    /// フレームごとにGPUを切り替えるレンダリング戦略
    class RHI_API RHIAlternateFrameRenderer
    {
    public:
        RHIAlternateFrameRenderer() = default;

        /// 初期化
        /// @param multiGPU マルチGPUデバイス
        void Initialize(IRHIMultiGPUDevice* multiGPU)
        {
            m_multiGPU = multiGPU;
            m_nodeCount = multiGPU ? multiGPU->GetNodeCount() : 1;
            m_currentFrame = 0;
        }

        /// 現在のフレームで使用するGPUノード取得
        ERHIGPUNode GetCurrentNode() const
        {
            return m_nodeCount > 1 ? static_cast<ERHIGPUNode>(m_currentFrame % m_nodeCount) : 0;
        }

        /// 現在フレームのノードアフィニティマスク取得
        RHINodeAffinityMask GetCurrentNodeMask() const { return RHINodeAffinityMask::Single(GetCurrentNode()); }

        /// 次フレームへ進む
        void AdvanceFrame() { ++m_currentFrame; }

        /// フレーム番号取得
        uint64 GetCurrentFrame() const { return m_currentFrame; }

        /// ノード数取得
        uint32 GetNodeCount() const { return m_nodeCount; }

    private:
        IRHIMultiGPUDevice* m_multiGPU = nullptr;
        uint32 m_nodeCount = 1;
        uint64 m_currentFrame = 0;
    };

    //=========================================================================
    // RHISplitFrameRenderer (19-04)
    //=========================================================================

    /// SFR（Split Frame Rendering）ヘルパー
    /// 画面を分割して複数GPUで同時レンダリングする戦略
    class RHI_API RHISplitFrameRenderer
    {
    public:
        /// 分割領域
        struct SplitRegion
        {
            ERHIGPUNode node = 0;
            RHIRect rect;
        };

        RHISplitFrameRenderer() = default;

        /// 初期化
        /// @param multiGPU マルチGPUデバイス
        /// @param screenWidth 画面幅
        /// @param screenHeight 画面高さ
        void Initialize(IRHIMultiGPUDevice* multiGPU, uint32 screenWidth, uint32 screenHeight)
        {
            m_multiGPU = multiGPU;
            m_nodeCount = multiGPU ? multiGPU->GetNodeCount() : 1;
            m_screenWidth = screenWidth;
            m_screenHeight = screenHeight;
        }

        /// 水平分割時の指定ノードの領域取得
        SplitRegion GetHorizontalSplitRegion(ERHIGPUNode node) const
        {
            SplitRegion region;
            region.node = node;
            uint32 sliceHeight = m_screenHeight / m_nodeCount;
            int32 top = static_cast<int32>(node * sliceHeight);
            int32 bottom = (node == m_nodeCount - 1) ? static_cast<int32>(m_screenHeight)
                                                     : static_cast<int32>((node + 1) * sliceHeight);
            region.rect = RHIRect(0, top, static_cast<int32>(m_screenWidth), bottom);
            return region;
        }

        /// 垂直分割時の指定ノードの領域取得
        SplitRegion GetVerticalSplitRegion(ERHIGPUNode node) const
        {
            SplitRegion region;
            region.node = node;
            uint32 sliceWidth = m_screenWidth / m_nodeCount;
            int32 left = static_cast<int32>(node * sliceWidth);
            int32 right = (node == m_nodeCount - 1) ? static_cast<int32>(m_screenWidth)
                                                    : static_cast<int32>((node + 1) * sliceWidth);
            region.rect = RHIRect(left, 0, right, static_cast<int32>(m_screenHeight));
            return region;
        }

        /// ノード数取得
        uint32 GetNodeCount() const { return m_nodeCount; }

    private:
        IRHIMultiGPUDevice* m_multiGPU = nullptr;
        uint32 m_nodeCount = 1;
        uint32 m_screenWidth = 0;
        uint32 m_screenHeight = 0;
    };

} // namespace NS::RHI
