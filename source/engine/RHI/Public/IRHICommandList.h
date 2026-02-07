/// @file IRHICommandList.h
/// @brief コマンドリストインターフェース
/// @details 記録されたGPUコマンドを保持するオブジェクト。ライフサイクル、バンドル、プール、統計を提供。
/// @see 02-05-command-list.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIResourceType.h"

namespace NS::RHI
{
    //=========================================================================
    // ERHICommandListState
    //=========================================================================

    /// コマンドリスト状態
    enum class ERHICommandListState : uint8
    {
        Initial,   ///< 初期状態（リセット後）
        Recording, ///< 記録中
        Closed,    ///< クローズ済み（実行可能）
        Pending,   ///< 実行待機
        Executing, ///< 実行中
    };

    //=========================================================================
    // ERHICommandListType
    //=========================================================================

    /// コマンドリストタイプ
    enum class ERHICommandListType : uint8
    {
        Direct, ///< 直接コマンドリスト（プライマリ）
        Bundle, ///< バンドル（セカンダリ、再利用可能）
    };

    //=========================================================================
    // RHICommandListStats
    //=========================================================================

    /// コマンドリスト統計
    struct RHICommandListStats
    {
        uint32 commandCount = 0; ///< 記録されたコマンド数（概算）
        uint32 drawCalls = 0;    ///< 描画呼び出し数
        uint32 dispatches = 0;   ///< ディスパッチ数
        uint32 barriers = 0;     ///< バリア数
        uint64 memoryUsed = 0;   ///< 使用メモリ（バイト）
    };

    //=========================================================================
    // IRHICommandList
    //=========================================================================

    /// コマンドリスト
    /// 記録されたGPUコマンドを保持するオブジェクト
    class RHI_API IRHICommandList : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(CommandList)

        virtual ~IRHICommandList() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// 対応するキュータイプ取得
        virtual ERHIQueueType GetQueueType() const = 0;

        /// 現在の状態取得
        virtual ERHICommandListState GetState() const = 0;

        /// コマンドリストタイプ取得
        virtual ERHICommandListType GetListType() const = 0;

        //=====================================================================
        // ライフサイクル
        //=====================================================================

        /// 記録開始
        /// @param allocator 使用するコマンドアロケーター
        /// @param initialPSO 初期パイプラインステート（nullptr可）
        virtual void Reset(IRHICommandAllocator* allocator, IRHIPipelineState* initialPSO = nullptr) = 0;

        /// 記録終了
        /// @note 記録完了後実行可能状態になる
        virtual void Close() = 0;

        /// 記録中か
        bool IsRecording() const { return GetState() == ERHICommandListState::Recording; }

        /// 実行可能か
        bool IsExecutable() const { return GetState() == ERHICommandListState::Closed; }

        //=====================================================================
        // アロケーター
        //=====================================================================

        /// 使用中のアロケーター取得
        virtual IRHICommandAllocator* GetAllocator() const = 0;

        /// 使用したコマンドメモリサイズ取得
        virtual uint64 GetUsedMemory() const = 0;

        //=====================================================================
        // バンドル
        //=====================================================================

        /// バンドルか
        bool IsBundle() const { return GetListType() == ERHICommandListType::Bundle; }

        /// バンドル実行
        /// @param bundle 実行するバンドル
        /// @note Directコマンドリストからのみ呼び出し可能
        virtual void ExecuteBundle(IRHICommandList* bundle) = 0;

        //=====================================================================
        // 統計
        //=====================================================================

        /// 統計情報取得
        virtual RHICommandListStats GetStats() const = 0;
    };

    //=========================================================================
    // IRHICommandListPool
    //=========================================================================

    /// コマンドリストプール
    /// コマンドリストの再利用管理
    class RHI_API IRHICommandListPool
    {
    public:
        virtual ~IRHICommandListPool() = default;

        /// コマンドリスト取得
        /// @param allocator 使用するアロケーター
        /// @param type リストタイプ
        /// @return コマンドリスト
        virtual IRHICommandList* Obtain(IRHICommandAllocator* allocator,
                                        ERHICommandListType type = ERHICommandListType::Direct) = 0;

        /// コマンドリスト返却
        /// @param commandList 返却するコマンドリスト
        virtual void Release(IRHICommandList* commandList) = 0;

        /// プール内のリスト数
        virtual uint32 GetPooledCount() const = 0;
    };

} // namespace NS::RHI
