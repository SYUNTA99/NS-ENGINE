/// @file IRHIFence.h
/// @brief フェンスインターフェース
/// @details GPU同期のためのフェンス、フェンス値トラッカーを提供。
/// @see 09-01-fence-interface.md
#pragma once

#include "IRHIResource.h"
#include "RHIMacros.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // RHIFenceDesc (09-01)
    //=========================================================================

    /// フェンス記述
    struct RHI_API RHIFenceDesc
    {
        uint64 initialValue = 0; ///< 初期値

        /// フラグ
        enum class Flags : uint32
        {
            None = 0,
            Shared = 1 << 0,         ///< 共有フェンス（プロセス間共有可能）
            CrossAdapter = 1 << 1,   ///< クロスアダプター共有
            MonitoredFence = 1 << 2, ///< モニター対応
        };
        Flags flags = Flags::None;
    };
    RHI_ENUM_CLASS_FLAGS(RHIFenceDesc::Flags)

    //=========================================================================
    // IRHIFence (09-01)
    //=========================================================================

    /// フェンス
    /// GPU-CPU間およびキュー間の同期プリミティブ
    class RHI_API IRHIFence : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Fence)

        virtual ~IRHIFence() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        virtual IRHIDevice* GetDevice() const = 0;

        /// 現在の完了値を取得
        virtual uint64 GetCompletedValue() const = 0;

        /// 最後にシグナルされた値
        virtual uint64 GetLastSignaledValue() const = 0;

        //=====================================================================
        // シグナル・待機
        //=====================================================================

        /// CPU側でシグナル（テスト用）
        virtual void Signal(uint64 value) = 0;

        /// CPU側で待機
        /// @param value 待機する値
        /// @param timeoutMs タイムアウト（ミリ秒、UINT64_MAXで無限待機）
        /// @return 成功したか（タイムアウトでfalse）
        virtual bool Wait(uint64 value, uint64 timeoutMs = UINT64_MAX) = 0;

        /// 待機（複数値のいずれか）
        virtual bool WaitAny(const uint64* values, uint32 count, uint64 timeoutMs = UINT64_MAX) = 0;

        /// 待機（複数値のすべて）
        virtual bool WaitAll(const uint64* values, uint32 count, uint64 timeoutMs = UINT64_MAX) = 0;

        //=====================================================================
        // イベント
        //=====================================================================

        /// 完了時イベント通知設定
        virtual void SetEventOnCompletion(uint64 value, void* eventHandle) = 0;

        //=====================================================================
        // 共有
        //=====================================================================

        /// 共有ハンドル取得（プロセス間共有用）
        virtual void* GetSharedHandle() const = 0;

        //=====================================================================
        // ユーティリティ
        //=====================================================================

        /// 指定値が完了しているか
        bool IsCompleted(uint64 value) const { return GetCompletedValue() >= value; }

        /// 即座にポーリング（待機なし）
        bool Poll(uint64 value) const { return IsCompleted(value); }
    };

    using RHIFenceRef = TRefCountPtr<IRHIFence>;

    //=========================================================================
    // RHIFenceValueTracker (09-01)
    //=========================================================================

    /// フェンス値管理ヘルパー
    class RHI_API RHIFenceValueTracker
    {
    public:
        RHIFenceValueTracker() = default;

        /// 初期化
        void Initialize(IRHIFence* fence)
        {
            m_fence = fence;
            m_nextValue = fence->GetCompletedValue() + 1;
        }

        /// 次の値取得（インクリメント）
        uint64 GetNextValue() { return m_nextValue++; }

        /// 現在の次の値（インクリメントなし）
        uint64 PeekNextValue() const { return m_nextValue; }

        /// キューでシグナルし、値を返す
        uint64 Signal(IRHIQueue* queue);

        /// CPU待機
        bool WaitCPU(uint64 value, uint64 timeoutMs = UINT64_MAX) { return m_fence->Wait(value, timeoutMs); }

        /// 最新完了値取得
        uint64 GetCompletedValue() const { return m_fence->GetCompletedValue(); }

        /// 指定値が完了しているか
        bool IsCompleted(uint64 value) const { return m_fence->IsCompleted(value); }

        /// フェンス取得
        IRHIFence* GetFence() const { return m_fence; }

    private:
        IRHIFence* m_fence = nullptr;
        uint64 m_nextValue = 1;
    };

}} // namespace NS::RHI
