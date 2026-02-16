/// @file RHIBreadcrumbs.h
/// @brief GPU Breadcrumbsシステム
/// @details GPUコマンド実行履歴追跡、クラッシュ診断用の
///          Breadcrumbデータ、ノード、アロケーター、状態管理、RAIIスコープを提供。
/// @see 17-04-breadcrumbs.md
#pragma once

#include "Common/Utility/Macros.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHITypes.h"

//=============================================================================
// ビルド設定
//=============================================================================

#ifndef NS_RHI_BREADCRUMBS_FULL
#if NS_BUILD_DEBUG || defined(NS_DEVELOPMENT)
#define NS_RHI_BREADCRUMBS_FULL 1
#define NS_RHI_BREADCRUMBS_MINIMAL 0
#elif defined(NS_PROFILE_GPU)
#define NS_RHI_BREADCRUMBS_FULL 0
#define NS_RHI_BREADCRUMBS_MINIMAL 1
#else
#define NS_RHI_BREADCRUMBS_FULL 0
#define NS_RHI_BREADCRUMBS_MINIMAL 0
#endif
#endif

/// CPUトレース出力
#define NS_RHI_BREADCRUMBS_EMIT_CPU (NS_RHI_BREADCRUMBS_FULL)

/// ソース位置情報
#define NS_RHI_BREADCRUMBS_EMIT_LOCATION (NS_RHI_BREADCRUMBS_FULL)

namespace NS { namespace RHI {
    //=========================================================================
    // RHIBreadcrumbData (17-04)
    //=========================================================================

    /// Breadcrumbのメタデータ
    struct RHIBreadcrumbData
    {
        /// 静的名前（コンパイル時定数）
        const char* staticName = nullptr;

        /// ソースファイル
        const char* sourceFile = nullptr;

        /// 行番号
        uint32 sourceLine = 0;

        /// GPU統計ID（プロファイリング連携用）
        uint32 statsId = 0;
    };

    //=========================================================================
    // RHIBreadcrumbNode (17-04)
    //=========================================================================

    /// Breadcrumbノード
    struct RHI_API RHIBreadcrumbNode
    {
        /// 一意のID
        uint32 id = 0;

        /// 親ノード（nullptr = ルート）
        RHIBreadcrumbNode* parent = nullptr;

        /// メタデータ
        const RHIBreadcrumbData* data = nullptr;

        /// フルパス取得（デバッグ用）
        /// @param outBuffer 出力バッファ
        /// @param bufferSize バッファサイズ
        /// @return 書き込まれた文字数
        uint32 GetFullPath(char* outBuffer, uint32 bufferSize) const;

        /// クラッシュデータ書き込み
        /// @param outBuffer 出力バッファ
        /// @param bufferSize バッファサイズ
        /// @return 書き込まれた文字数
        uint32 WriteCrashData(char* outBuffer, uint32 bufferSize) const;
    };

    //=========================================================================
    // RHIBreadcrumbAllocator (17-04)
    //=========================================================================

    /// Breadcrumb専用アロケーター
    class RHI_API RHIBreadcrumbAllocator
    {
    public:
        RHIBreadcrumbAllocator() = default;
        ~RHIBreadcrumbAllocator() = default;

        /// 初期化
        bool Initialize(uint32 maxNodes = 4096);

        /// シャットダウン
        void Shutdown();

        /// ノードアロケート
        RHIBreadcrumbNode* AllocateNode(RHIBreadcrumbNode* parent, const RHIBreadcrumbData& data);

        /// フレーム終了時リセット
        void Reset();

        /// 使用中ノード数
        uint32 GetAllocatedCount() const { return m_nextId; }

    private:
        RHIBreadcrumbNode* m_nodes = nullptr;
        uint32 m_capacity = 0;
        uint32 m_nextId = 0;
    };

    //=========================================================================
    // RHIBreadcrumbState (17-04)
    //=========================================================================

    /// Breadcrumb状態（スレッドローカル）
    class RHI_API RHIBreadcrumbState
    {
    public:
        /// 現在のスレッドのインスタンス取得
        static RHIBreadcrumbState& Get();

        /// 現在のアクティブノード取得
        RHIBreadcrumbNode* GetCurrentNode() const;

        /// ノードをプッシュ
        void PushNode(RHIBreadcrumbNode* node);

        /// ノードをポップ
        void PopNode();

        /// アクティブなBreadcrumbをダンプ（クラッシュ時）
        static void DumpActiveBreadcrumbs();

    private:
        static constexpr uint32 kMaxStackDepth = 64;

        RHIBreadcrumbNode* m_nodeStack[kMaxStackDepth] = {};
        uint32 m_stackDepth = 0;
    };

    //=========================================================================
    // RHIBreadcrumbScope (17-04)
    //=========================================================================

    /// Breadcrumb RAIIスコープ
    class RHIBreadcrumbScope
    {
    public:
        NS_DISALLOW_COPY(RHIBreadcrumbScope);

        RHIBreadcrumbScope(IRHICommandContext* context,
                           RHIBreadcrumbAllocator* allocator,
                           const char* name,
                           const char* sourceFile = nullptr,
                           uint32 sourceLine = 0);

        ~RHIBreadcrumbScope();

    private:
        IRHICommandContext* m_context = nullptr;
        RHIBreadcrumbNode* m_node = nullptr;
    };

}} // namespace NS::RHI

//=============================================================================
// Breadcrumbマクロ (17-04)
//=============================================================================

#if NS_RHI_BREADCRUMBS_FULL || NS_RHI_BREADCRUMBS_MINIMAL

/// 基本Breadcrumbイベント
#define RHI_BREADCRUMB_EVENT(context, allocator, name)                                                                 \
    ::NS::RHI::RHIBreadcrumbScope NS_MACRO_CONCATENATE(_breadcrumb_,                                                   \
                                                       __LINE__)(context, allocator, name, __FILE__, __LINE__)

/// 条件付きBreadcrumbイベント
#define RHI_BREADCRUMB_EVENT_CONDITIONAL(context, allocator, condition, name)                                          \
    ::NS::RHI::RHIBreadcrumbScope NS_MACRO_CONCATENATE(_breadcrumb_, __LINE__)(                                        \
        (condition) ? (context) : nullptr, (condition) ? (allocator) : nullptr, name, __FILE__, __LINE__)

#else

#define RHI_BREADCRUMB_EVENT(context, allocator, name)
#define RHI_BREADCRUMB_EVENT_CONDITIONAL(context, allocator, condition, name)

#endif
