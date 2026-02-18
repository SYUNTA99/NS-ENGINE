/// @file GenericPlatformAffinity.h
/// @brief プラットフォーム非依存のスレッドアフィニティ・優先度管理インターフェース
#pragma once

#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"

namespace NS
{
    /// スレッド優先度
    ///
    /// OSのスレッド優先度にマッピングされる。
    /// 実際の優先度値はプラットフォーム依存。
    enum class ThreadPriority
    {
        TimeCritical,       ///< 最高優先度（オーディオ等）
        Highest,            ///< 非常に高い
        AboveNormal,        ///< 通常より高い
        Normal,             ///< 通常（デフォルト）
        BelowNormal,        ///< 通常より低い
        Lowest,             ///< 最低優先度
        SlightlyBelowNormal ///< 通常よりわずかに低い（バックグラウンド処理）
    };

    /// スレッド用途タイプ
    ///
    /// アフィニティマスク取得時の用途指定。
    /// プラットフォームごとに最適なコア配置を提供。
    enum class ThreadType
    {
        MainGame,   ///< メインゲームスレッド（コア0優先）
        Rendering,  ///< レンダリングスレッド
        RHI,        ///< RHIコマンド生成スレッド
        TaskGraph,  ///< タスクグラフワーカー
        Pool,       ///< 汎用スレッドプール
        Audio,      ///< オーディオ処理（低レイテンシ）
        Loading,    ///< アセットロード（I/Oバウンド）
        Background, ///< バックグラウンド処理（低優先度）
        Count
    };

    /// CPUトポロジ情報
    struct CPUTopology
    {
        uint32 physicalCoreCount{0};     ///< 物理コア数
        uint32 logicalProcessorCount{0}; ///< 論理プロセッサ数（HT含む）
        uint32 performanceCoreCount{0};  ///< Pコア数（ハイブリッドCPU）
        uint32 efficiencyCoreCount{0};   ///< Eコア数（ハイブリッドCPU）
        uint64 performanceCoreMask{0};   ///< Pコアのビットマスク
        uint64 efficiencyCoreMask{0};    ///< Eコアのビットマスク
        bool isHybridCPU{false};             ///< ハイブリッドCPU（Intel 12th+, ARM big.LITTLE）

        CPUTopology()
             
        {}
    };

    /// プラットフォーム非依存のアフィニティ管理
    ///
    /// ## スレッドセーフティ
    ///
    /// 全関数はスレッドセーフ（読み取り専用または内部同期）。
    ///
    /// ## アフィニティマスク
    ///
    /// ビットマスクで実行可能なCPUコアを指定。
    /// - bit N = 1: コアNで実行可能
    /// - 0xFFFFFFFFFFFFFFFF: 全コアで実行可能（制限なし）
    struct GenericPlatformAffinity
    {
        // =====================================================================
        // アフィニティマスク取得
        // =====================================================================

        /// スレッドタイプに応じたアフィニティマスク取得
        ///
        /// @param type スレッド用途
        /// @return アフィニティマスク
        static uint64 GetAffinityMask(ThreadType type);

        /// 制限なしマスク取得
        static uint64 GetNoAffinityMask() { return 0xFFFFFFFFFFFFFFFF; }

        /// 個別マスク（後方互換）
        static uint64 GetMainGameMask() { return GetAffinityMask(ThreadType::MainGame); }
        static uint64 GetRenderingThreadMask() { return GetAffinityMask(ThreadType::Rendering); }
        static uint64 GetRHIThreadMask() { return GetAffinityMask(ThreadType::RHI); }
        static uint64 GetTaskGraphThreadMask() { return GetAffinityMask(ThreadType::TaskGraph); }
        static uint64 GetPoolThreadMask() { return GetAffinityMask(ThreadType::Pool); }

        // =====================================================================
        // 優先度
        // =====================================================================

        /// スレッドタイプのデフォルト優先度取得
        static ThreadPriority GetDefaultPriority(ThreadType type);

        /// 汎用デフォルト優先度
        static ThreadPriority GetDefaultPriority() { return ThreadPriority::Normal; }

        // =====================================================================
        // トポロジ情報
        // =====================================================================

        /// CPUトポロジ取得
        ///
        /// @return CPUトポロジ情報
        /// @note 初回呼び出し時にキャッシュされる
        static const CPUTopology& GetCPUTopology();

        // =====================================================================
        // 実行時バインド
        // =====================================================================

        /// 現在のスレッドにアフィニティを設定
        ///
        /// @param mask アフィニティマスク
        /// @return 設定成功した場合true
        static bool SetCurrentThreadAffinity(uint64 mask);

        /// 現在のスレッドの優先度を設定
        ///
        /// @param priority 優先度
        /// @return 設定成功した場合true
        static bool SetCurrentThreadPriority(ThreadPriority priority);

        /// 現在のスレッドが実行中のコアIDを取得
        ///
        /// @return コアID（0〜logicalProcessorCount-1）
        static uint32 GetCurrentProcessorNumber();

        /// スレッドをスリープ（ミリ秒）
        static void Sleep(uint32 milliseconds);

        /// 他のスレッドに実行権を譲る
        static void YieldThread();
    };
} // namespace NS
