/// @file RHIGPUReadback.h
/// @brief GPUバッファリードバック
/// @details 非同期バッファリードバック、型安全ヘルパー、配列リードバックを提供。
/// @see 20-02-gpu-readback.md
#pragma once

#include "Common/Result/Core/Result.h"
#include "IRHIResource.h"
#include "RHIMacros.h"
#include "RHITypes.h"

#include <utility>

namespace NS { namespace RHI {
    //=========================================================================
    // ERHIReadbackState (20-02)
    //=========================================================================

    /// リードバック状態
    enum class ERHIReadbackState : uint8
    {
        Pending,  ///< リクエスト発行前
        InFlight, ///< GPU処理中
        Ready,    ///< データ取得可能
        Failed    ///< 失敗
    };

    //=========================================================================
    // RHIBufferReadbackDesc (20-02)
    //=========================================================================

    /// バッファリードバック記述
    struct RHIBufferReadbackDesc
    {
        uint64 maxSize = 0; ///< 最大リードバックサイズ
        const char* debugName = nullptr;
    };

    //=========================================================================
    // IRHIBufferReadback (20-02)
    //=========================================================================

    /// バッファリードバック
    /// 非同期的にGPUバッファのデータをCPUへ読み戻す
    class RHI_API IRHIBufferReadback : public IRHIResource
    {
    public:
        virtual ~IRHIBufferReadback() = default;

        //=====================================================================
        // リードバック操作
        //=====================================================================

        /// リードバックを開始（コマンドリストに記録）
        /// @param context 記録先コンテキスト
        /// @param sourceBuffer 読み取り元バッファ
        /// @param sourceOffset ソースのオフセット
        /// @param size 読み取りサイズ
        virtual void EnqueueCopy(IRHICommandContext* context,
                                 IRHIBuffer* sourceBuffer,
                                 uint64 sourceOffset,
                                 uint64 size) = 0;

        //=====================================================================
        // 状態管理
        //=====================================================================

        /// 現在の状態取得
        virtual ERHIReadbackState GetState() const = 0;

        /// データが準備完了か（ノンブロッキング）
        virtual bool IsReady() const = 0;

        /// データが準備完了になるまで待つ
        /// @param timeoutMs タイムアウト（0=無限待機）
        /// @return 成功でtrue、タイムアウトでfalse
        virtual bool Wait(uint32 timeoutMs = 0) = 0;

        //=====================================================================
        // データ取得
        //=====================================================================

        /// 読み取ったデータサイズ
        virtual uint64 GetDataSize() const = 0;

        /// データをコピー取得
        /// @param outData 出力バッファ
        /// @param size 読み取りサイズ
        /// @param offset オフセット
        /// @return 成功でOK
        virtual NS::Result GetData(void* outData, uint64 size, uint64 offset = 0) = 0;

        /// マップしてポインタ取得（ゼロコピー）
        /// @return マップされたポインタ、未準備ならnullptr
        virtual const void* Lock() = 0;

        /// マップ解除
        virtual void Unlock() = 0;
    };

    using RHIBufferReadbackRef = TRefCountPtr<IRHIBufferReadback>;

    //=========================================================================
    // TRHITypedReadback (20-02)
    //=========================================================================

    /// 型付きバッファリードバック
    template <typename T> class TRHITypedReadback
    {
    public:
        explicit TRHITypedReadback(RHIBufferReadbackRef readback) : m_readback(std::move(readback)) {}

        void EnqueueCopy(IRHICommandContext* context, IRHIBuffer* source, uint64 offset = 0)
        {
            m_readback->EnqueueCopy(context, source, offset, sizeof(T));
        }

        bool IsReady() const { return m_readback->IsReady(); }
        bool Wait(uint32 timeoutMs = 0) { return m_readback->Wait(timeoutMs); }

        NS::Result GetValue(T& outValue) { return m_readback->GetData(&outValue, sizeof(T)); }

        T GetValueOrDefault(const T& defaultValue = T{})
        {
            T value;
            if (m_readback->GetData(&value, sizeof(T)).IsOk())
            {
                return value;
            }
            return defaultValue;
        }

    private:
        RHIBufferReadbackRef m_readback;
    };

    //=========================================================================
    // TRHIArrayReadback (20-02)
    //=========================================================================

    /// 配列リードバック
    template <typename T> class TRHIArrayReadback
    {
    public:
        explicit TRHIArrayReadback(RHIBufferReadbackRef readback) : m_readback(std::move(readback)) {}

        void EnqueueCopy(IRHICommandContext* context, IRHIBuffer* source, uint32 count, uint64 offset = 0)
        {
            m_count = count;
            m_readback->EnqueueCopy(context, source, offset, sizeof(T) * count);
        }

        bool IsReady() const { return m_readback->IsReady(); }
        bool Wait(uint32 timeoutMs = 0) { return m_readback->Wait(timeoutMs); }
        uint32 GetCount() const { return m_count; }

        NS::Result GetValues(T* outValues, uint32 count) { return m_readback->GetData(outValues, sizeof(T) * count); }

        /// スパン取得（ロック必要）
        class LockedSpan
        {
        public:
            LockedSpan(IRHIBufferReadback* readback, uint32 count)
                : m_readback(readback), m_data(static_cast<const T*>(readback->Lock())), m_count(m_data ? count : 0)
            {}

            ~LockedSpan()
            {
                if (m_data)
                {
                    m_readback->Unlock();
                }
            }

            const T* begin() const { return m_data; }
            const T* end() const { return m_data + m_count; }
            uint32 size() const { return m_count; }
            const T& operator[](uint32 index) const { return m_data[index]; }
            bool IsValid() const { return m_data != nullptr; }

        private:
            IRHIBufferReadback* m_readback;
            const T* m_data;
            uint32 m_count;
        };

        LockedSpan Lock() { return LockedSpan(m_readback.Get(), m_count); }

    private:
        RHIBufferReadbackRef m_readback;
        uint32 m_count = 0;
    };

}} // namespace NS::RHI
