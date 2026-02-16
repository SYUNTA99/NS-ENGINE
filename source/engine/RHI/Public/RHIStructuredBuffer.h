/// @file RHIStructuredBuffer.h
/// @brief 構造化バッファ・バイトアドレスバッファ・インダイレクト引数バッファ・定数バッファの高レベルラッパー
/// @see 03-04-structured-buffer.md
#pragma once

#include "IRHIBuffer.h"
#include "RHIRefCountPtr.h"

namespace NS { namespace RHI {
    //=========================================================================
    // RHIStructuredBuffer (03-04)
    //=========================================================================

    /// 型付き構造化バッファラッパー
    /// StructuredBuffer<T> / RWStructuredBuffer<T> に対応
    template <typename T> class RHIStructuredBuffer
    {
    public:
        using ElementType = T;

        RHIStructuredBuffer() = default;

        explicit RHIStructuredBuffer(RHIBufferRef buffer) : m_buffer(std::move(buffer))
        {
            RHI_CHECK(!m_buffer || m_buffer->GetStride() == sizeof(T));
        }

        /// 作成
        static RHIStructuredBuffer Create(IRHIDevice* device,
                                          uint32 elementCount,
                                          ERHIBufferUsage additionalUsage = ERHIBufferUsage::None,
                                          const char* debugName = nullptr);

        /// RWStructuredBuffer用に作成
        static RHIStructuredBuffer CreateRW(IRHIDevice* device, uint32 elementCount, const char* debugName = nullptr);

        bool IsValid() const { return m_buffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        uint32 GetCount() const { return m_buffer ? m_buffer->GetElementCount() : 0; }
        MemorySize GetSize() const { return m_buffer ? m_buffer->GetSize() : 0; }
        static constexpr uint32 GetStride() { return sizeof(T); }

        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        RHIBufferRef GetBufferRef() const { return m_buffer; }
        uint64 GetGPUVirtualAddress() const { return m_buffer ? m_buffer->GetGPUVirtualAddress() : 0; }

        RHITypedBufferLock<T> Lock(ERHIMapMode mode) { return RHITypedBufferLock<T>(m_buffer.Get(), mode); }
        RHITypedBufferLock<const T> LockRead()
        {
            return RHITypedBufferLock<const T>(m_buffer.Get(), ERHIMapMode::Read);
        }

        bool WriteAll(const T* data, uint32 count)
        {
            if (!m_buffer || count > GetCount())
                return false;
            return m_buffer->WriteArray(data, count);
        }

        bool WriteAt(uint32 index, const T& value)
        {
            if (!m_buffer || index >= GetCount())
                return false;
            return m_buffer->Write(value, index * sizeof(T));
        }

        bool ReadAt(uint32 index, T& outValue)
        {
            if (!m_buffer || index >= GetCount())
                return false;
            return m_buffer->Read(outValue, index * sizeof(T));
        }

    private:
        RHIBufferRef m_buffer;
    };

    //=========================================================================
    // RHIAppendBuffer (03-04)
    //=========================================================================

    /// Append/Consumeバッファ用カウンター付き構造化バッファ
    template <typename T> class RHIAppendBuffer
    {
    public:
        using ElementType = T;

        RHIAppendBuffer() = default;

        static RHIAppendBuffer Create(IRHIDevice* device, uint32 maxElementCount, const char* debugName = nullptr);

        bool IsValid() const { return m_buffer != nullptr && m_counterBuffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        uint32 GetMaxCount() const { return m_buffer ? m_buffer->GetElementCount() : 0; }
        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        IRHIBuffer* GetCounterBuffer() const { return m_counterBuffer.Get(); }

        /// カウンターをリセット
        void ResetCounter(IRHICommandContext* context);

        /// カウンター値を読み取り（ステージング経由）
        uint32 ReadCounterValue();

        /// カウンターバッファのGPUアドレス取得
        uint64 GetCounterGPUAddress() const { return m_counterBuffer ? m_counterBuffer->GetGPUVirtualAddress() : 0; }

    private:
        RHIBufferRef m_buffer;
        RHIBufferRef m_counterBuffer;
    };

    //=========================================================================
    // RHIByteAddressBuffer (03-04)
    //=========================================================================

    /// バイトアドレスバッファラッパー
    class RHI_API RHIByteAddressBuffer
    {
    public:
        RHIByteAddressBuffer() = default;
        explicit RHIByteAddressBuffer(RHIBufferRef buffer) : m_buffer(std::move(buffer)) {}

        static RHIByteAddressBuffer Create(IRHIDevice* device,
                                           MemorySize size,
                                           ERHIBufferUsage additionalUsage = ERHIBufferUsage::None,
                                           const char* debugName = nullptr);

        static RHIByteAddressBuffer CreateRW(IRHIDevice* device, MemorySize size, const char* debugName = nullptr);

        bool IsValid() const { return m_buffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        MemorySize GetSize() const { return m_buffer ? m_buffer->GetSize() : 0; }
        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        RHIBufferRef GetBufferRef() const { return m_buffer; }
        uint64 GetGPUVirtualAddress() const { return m_buffer ? m_buffer->GetGPUVirtualAddress() : 0; }

        template <typename T> bool Load(MemoryOffset byteOffset, T& outValue)
        {
            return m_buffer ? m_buffer->Read(outValue, byteOffset) : false;
        }

        template <typename T> bool Store(MemoryOffset byteOffset, const T& value)
        {
            return m_buffer ? m_buffer->Write(value, byteOffset) : false;
        }

        bool Load4(MemoryOffset byteOffset, uint32 (&outValues)[4]) { return Load(byteOffset, outValues); }
        bool Store4(MemoryOffset byteOffset, const uint32 (&values)[4]) { return Store(byteOffset, values); }

    private:
        RHIBufferRef m_buffer;
    };

    //=========================================================================
    // Indirect引数構造体 (03-04)
    //=========================================================================

    /// DrawIndirect引数構造体
    struct RHI_API RHIDrawIndirectArgs
    {
        uint32 vertexCountPerInstance;
        uint32 instanceCount;
        uint32 startVertexLocation;
        uint32 startInstanceLocation;
    };

    /// DrawIndexedIndirect引数構造体
    struct RHI_API RHIDrawIndexedIndirectArgs
    {
        uint32 indexCountPerInstance;
        uint32 instanceCount;
        uint32 startIndexLocation;
        int32 baseVertexLocation;
        uint32 startInstanceLocation;
    };

    /// DispatchIndirect引数構造体
    struct RHI_API RHIDispatchIndirectArgs
    {
        uint32 threadGroupCountX;
        uint32 threadGroupCountY;
        uint32 threadGroupCountZ;
    };

    //=========================================================================
    // RHIIndirectArgsBuffer (03-04)
    //=========================================================================

    /// インダイレクト引数バッファ
    class RHI_API RHIIndirectArgsBuffer
    {
    public:
        RHIIndirectArgsBuffer() = default;
        explicit RHIIndirectArgsBuffer(RHIBufferRef buffer) : m_buffer(std::move(buffer)) {}

        static RHIIndirectArgsBuffer CreateForDraw(IRHIDevice* device,
                                                   uint32 maxDrawCount,
                                                   const char* debugName = nullptr);
        static RHIIndirectArgsBuffer CreateForDrawIndexed(IRHIDevice* device,
                                                          uint32 maxDrawCount,
                                                          const char* debugName = nullptr);
        static RHIIndirectArgsBuffer CreateForDispatch(IRHIDevice* device,
                                                       uint32 maxDispatchCount,
                                                       const char* debugName = nullptr);

        bool IsValid() const { return m_buffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        RHIBufferRef GetBufferRef() const { return m_buffer; }
        uint64 GetGPUVirtualAddress() const { return m_buffer ? m_buffer->GetGPUVirtualAddress() : 0; }

        bool SetDrawArgs(uint32 index, const RHIDrawIndirectArgs& args)
        {
            return m_buffer ? m_buffer->Write(args, index * sizeof(RHIDrawIndirectArgs)) : false;
        }

        bool SetDrawIndexedArgs(uint32 index, const RHIDrawIndexedIndirectArgs& args)
        {
            return m_buffer ? m_buffer->Write(args, index * sizeof(RHIDrawIndexedIndirectArgs)) : false;
        }

        bool SetDispatchArgs(uint32 index, const RHIDispatchIndirectArgs& args)
        {
            return m_buffer ? m_buffer->Write(args, index * sizeof(RHIDispatchIndirectArgs)) : false;
        }

        MemoryOffset GetDrawArgsOffset(uint32 index) const { return index * sizeof(RHIDrawIndirectArgs); }
        MemoryOffset GetDrawIndexedArgsOffset(uint32 index) const { return index * sizeof(RHIDrawIndexedIndirectArgs); }
        MemoryOffset GetDispatchArgsOffset(uint32 index) const { return index * sizeof(RHIDispatchIndirectArgs); }

    private:
        RHIBufferRef m_buffer;
    };

    //=========================================================================
    // RHIConstantBuffer (03-04)
    //=========================================================================

    /// 型付き定数バッファラッパー
    template <typename T> class RHIConstantBuffer
    {
    public:
        using DataType = T;
        static_assert(sizeof(T) <= 65536, "Constant buffer too large (max 64KB)");

        RHIConstantBuffer() = default;

        static RHIConstantBuffer Create(IRHIDevice* device, const char* debugName = nullptr);
        static RHIConstantBuffer CreateDynamic(IRHIDevice* device, const char* debugName = nullptr);

        bool IsValid() const { return m_buffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        RHIBufferRef GetBufferRef() const { return m_buffer; }
        uint64 GetGPUVirtualAddress() const { return m_buffer ? m_buffer->GetGPUVirtualAddress() : 0; }

        static constexpr MemorySize GetAlignedSize() { return GetConstantBufferSize<T>(); }

        bool Update(const T& data) { return m_buffer ? m_buffer->Write(data) : false; }

        template <typename MemberType> bool UpdateMember(MemberType T::* member, const MemberType& value)
        {
            if (!m_buffer)
                return false;
            MemoryOffset offset = reinterpret_cast<MemoryOffset>(&(static_cast<T*>(nullptr)->*member));
            return m_buffer->Write(value, offset);
        }

    private:
        RHIBufferRef m_buffer;
    };

}} // namespace NS::RHI
