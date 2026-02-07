/// @file IRHIBuffer.h
/// @brief バッファリソースインターフェース
/// @details バッファ記述、コアインターフェース、Map/Unmap、スコープロック、書き込み/読み取りヘルパーを提供。
/// @see 03-01-buffer-desc.md, 03-02-buffer-interface.md, 03-03-buffer-lock.md
#pragma once

#include "Common/Utility/Macros.h"
#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIMacros.h"
#include "RHIPixelFormat.h"
#include "RHIResourceType.h"
#include "RHITypes.h"
#include <cstring>

namespace NS::RHI
{
    // 前方宣言
    struct RHIVertexBufferView;
    struct RHIIndexBufferView;

    //=========================================================================
    // RHIBufferDesc (03-01)
    //=========================================================================

    /// バッファ作成記述
    struct RHI_API RHIBufferDesc
    {
        MemorySize size = 0;                              ///< バッファサイズ（バイト）
        ERHIBufferUsage usage = ERHIBufferUsage::Default; ///< 使用フラグ
        uint32 stride = 0;                                ///< 要素ストライド（構造化バッファ用、0=非構造化）
        uint32 alignment = 0;                             ///< メモリアライメント要求（0=デフォルト）
        GPUMask gpuMask = GPUMask::GPU0();                ///< 対象GPU
        const char* debugName = nullptr;                  ///< デバッグ名

        // ビルダーパターン
        RHIBufferDesc& SetSize(MemorySize s)
        {
            size = s;
            return *this;
        }
        RHIBufferDesc& SetUsage(ERHIBufferUsage u)
        {
            usage = u;
            return *this;
        }
        RHIBufferDesc& SetStride(uint32 s)
        {
            stride = s;
            return *this;
        }
        RHIBufferDesc& SetAlignment(uint32 a)
        {
            alignment = a;
            return *this;
        }
        RHIBufferDesc& SetGPUMask(GPUMask m)
        {
            gpuMask = m;
            return *this;
        }
        RHIBufferDesc& SetDebugName(const char* name)
        {
            debugName = name;
            return *this;
        }
    };

    //=========================================================================
    // バッファ作成ヘルパー (03-01)
    //=========================================================================

    /// 頂点バッファDesc作成
    inline RHIBufferDesc CreateVertexBufferDesc(MemorySize size, uint32 stride, bool dynamic = false)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.stride = stride;
        desc.usage = dynamic ? ERHIBufferUsage::DynamicVertexBuffer : ERHIBufferUsage::VertexBuffer;
        return desc;
    }

    /// インデックスバッファDesc作成
    inline RHIBufferDesc CreateIndexBufferDesc(MemorySize size,
                                               ERHIIndexFormat format = ERHIIndexFormat::UInt16,
                                               bool dynamic = false)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.stride = GetIndexFormatSize(format);
        desc.usage = dynamic ? ERHIBufferUsage::DynamicIndexBuffer : ERHIBufferUsage::IndexBuffer;
        return desc;
    }

    /// 定数バッファDesc作成
    inline RHIBufferDesc CreateConstantBufferDesc(MemorySize size, bool dynamic = true)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.usage = dynamic ? ERHIBufferUsage::DynamicConstantBuffer : ERHIBufferUsage::ConstantBuffer;
        desc.alignment = kConstantBufferAlignment;
        return desc;
    }

    /// 構造化バッファDesc作成
    inline RHIBufferDesc CreateStructuredBufferDesc(MemorySize size,
                                                    uint32 stride,
                                                    ERHIBufferUsage additionalUsage = ERHIBufferUsage::None)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.stride = stride;
        desc.usage = ERHIBufferUsage::StructuredBuffer | ERHIBufferUsage::ShaderResource | additionalUsage;
        return desc;
    }

    /// UAV付き構造化バッファDesc作成
    inline RHIBufferDesc CreateRWStructuredBufferDesc(MemorySize size, uint32 stride)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.stride = stride;
        desc.usage =
            ERHIBufferUsage::StructuredBuffer | ERHIBufferUsage::ShaderResource | ERHIBufferUsage::UnorderedAccess;
        return desc;
    }

    /// ステージングバッファDesc作成
    inline RHIBufferDesc CreateStagingBufferDesc(MemorySize size)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.usage = ERHIBufferUsage::Staging;
        return desc;
    }

    /// 間接引数バッファDesc作成
    inline RHIBufferDesc CreateIndirectArgsBufferDesc(MemorySize size)
    {
        RHIBufferDesc desc;
        desc.size = size;
        desc.usage = ERHIBufferUsage::IndirectArgs | ERHIBufferUsage::ShaderResource | ERHIBufferUsage::UnorderedAccess;
        return desc;
    }

    //=========================================================================
    // RHIBufferInitData / RHIBufferCreateInfo (03-01)
    //=========================================================================

    /// バッファ初期化データ
    struct RHI_API RHIBufferInitData
    {
        const void* data = nullptr; ///< データポインタ
        MemorySize size = 0;        ///< データサイズ（0=バッファ全体）
        MemoryOffset offset = 0;    ///< バッファ内のオフセット
    };

    /// 作成と同時に初期化するバッファDesc
    struct RHI_API RHIBufferCreateInfo
    {
        RHIBufferDesc desc;
        RHIBufferInitData initData;

        RHIBufferCreateInfo& SetInitData(const void* data, MemorySize size = 0)
        {
            initData.data = data;
            initData.size = size;
            return *this;
        }

        template <typename T, size_t N> RHIBufferCreateInfo& SetInitData(const T (&array)[N])
        {
            initData.data = array;
            initData.size = sizeof(T) * N;
            return *this;
        }

        template <typename Container> RHIBufferCreateInfo& SetInitDataFromContainer(const Container& container)
        {
            initData.data = container.data();
            initData.size = container.size() * sizeof(typename Container::value_type);
            return *this;
        }
    };

    //=========================================================================
    // 定数バッファアライメント (03-01)
    // kConstantBufferAlignment は RHITypes.h で定義済み
    //=========================================================================

    /// 定数バッファサイズをアライメント
    inline constexpr MemorySize AlignConstantBufferSize(MemorySize size)
    {
        return AlignUp(size, kConstantBufferAlignment);
    }

    /// 構造体サイズから定数バッファサイズを計算
    template <typename T> inline constexpr MemorySize GetConstantBufferSize()
    {
        return AlignConstantBufferSize(sizeof(T));
    }

    /// 配列の定数バッファサイズを計算
    template <typename T> inline constexpr MemorySize GetConstantBufferArraySize(uint32 count)
    {
        return AlignConstantBufferSize(sizeof(T)) * count;
    }

    //=========================================================================
    // RHIMapResult (03-03)
    //=========================================================================

    /// マップ結果
    struct RHI_API RHIMapResult
    {
        void* data = nullptr;  ///< マップされたメモリポインタ
        MemorySize size = 0;   ///< マップされた領域のサイズ
        uint32 rowPitch = 0;   ///< 行ピッチ（テクスチャ用、バッファでは0）
        uint32 depthPitch = 0; ///< スライスピッチ（3Dテクスチャ用、バッファでは0）

        bool IsValid() const { return data != nullptr; }

        template <typename T> T* As() const { return static_cast<T*>(data); }
    };

    //=========================================================================
    // RHIBufferMemoryInfo (03-02)
    //=========================================================================

    /// バッファメモリ情報
    struct RHI_API RHIBufferMemoryInfo
    {
        MemorySize allocatedSize = 0; ///< 実際に割り当てられたサイズ
        MemorySize usableSize = 0;    ///< 使用可能サイズ（要求サイズ）
        MemoryOffset heapOffset = 0;  ///< ヒープ内オフセット
        ERHIHeapType heapType = ERHIHeapType::Default;
        uint32 alignment = 0;
    };

    //=========================================================================
    // RHIBufferViewInfo (03-02)
    //=========================================================================

    /// バッファビュー作成情報
    struct RHI_API RHIBufferViewInfo
    {
        IRHIBuffer* buffer = nullptr;
        MemoryOffset offset = 0;                           ///< 開始オフセット（バイト）
        MemorySize size = 0;                               ///< サイズ（0=バッファ全体）
        ERHIPixelFormat format = ERHIPixelFormat::Unknown; ///< フォーマット（型付きバッファ用）
    };

    //=========================================================================
    // IRHIBuffer (03-02 + 03-03)
    //=========================================================================

    /// バッファリソース
    /// GPU上のリニアメモリ領域
    class RHI_API IRHIBuffer : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Buffer)

        virtual ~IRHIBuffer() = default;

        //=====================================================================
        // ライフサイクル契約
        //=====================================================================
        //
        // - IRHIBufferはTRefCountPtrで管理される
        // - 参照カウントが0になるとOnZeroRefCount()が呼ばれ、
        //   RHIDeferredDeleteQueue::Enqueue()によりGPU完了まで削除を遅延する
        // - GPU操作が完了するまでバッファメモリは解放されない
        // - Lock/Unlockはスレッドセーフではない。
        //   複数スレッドからの同時アクセスには外部同期が必要
        //

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// バッファサイズ取得（バイト）
        virtual MemorySize GetSize() const = 0;

        /// 使用フラグ取得
        virtual ERHIBufferUsage GetUsage() const = 0;

        /// 要素ストライド取得（構造化バッファ用）
        virtual uint32 GetStride() const = 0;

        /// 要素数取得（構造化バッファ用）
        uint32 GetElementCount() const
        {
            uint32 s = GetStride();
            return s > 0 ? static_cast<uint32>(GetSize() / s) : 0;
        }

        //=====================================================================
        // GPUアドレス
        //=====================================================================

        /// GPUバーチャルアドレス取得
        virtual uint64 GetGPUVirtualAddress() const = 0;

        /// GPU上の有効アドレス範囲取得
        virtual void GetGPUAddressRange(uint64& outAddress, MemorySize& outSize) const
        {
            outAddress = GetGPUVirtualAddress();
            outSize = GetSize();
        }

        //=====================================================================
        // 使用フラグ判定
        //=====================================================================

        bool IsVertexBuffer() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::VertexBuffer); }
        bool IsIndexBuffer() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::IndexBuffer); }
        bool IsConstantBuffer() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::ConstantBuffer); }
        bool CanCreateSRV() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::ShaderResource); }
        bool CanCreateUAV() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::UnorderedAccess); }
        bool IsStructuredBuffer() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::StructuredBuffer); }
        bool IsByteAddressBuffer() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::ByteAddressBuffer); }
        bool IsIndirectArgsBuffer() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::IndirectArgs); }
        bool IsDynamic() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::Dynamic); }
        bool IsCPUWritable() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::CPUWritable); }
        bool IsCPUReadable() const { return EnumHasAnyFlags(GetUsage(), ERHIBufferUsage::CPUReadable); }

        //=====================================================================
        // メモリ情報
        //=====================================================================

        /// メモリ情報取得
        virtual RHIBufferMemoryInfo GetMemoryInfo() const = 0;

        MemorySize GetAllocatedSize() const { return GetMemoryInfo().allocatedSize; }
        ERHIHeapType GetHeapType() const { return GetMemoryInfo().heapType; }

        //=====================================================================
        // ビュー作成用
        //=====================================================================

        /// 全体をカバーするビュー情報取得
        RHIBufferViewInfo GetFullViewInfo() const
        {
            RHIBufferViewInfo info;
            info.buffer = const_cast<IRHIBuffer*>(this);
            info.offset = 0;
            info.size = GetSize();
            return info;
        }

        /// 部分ビュー情報取得
        RHIBufferViewInfo GetSubViewInfo(MemoryOffset offset, MemorySize size) const
        {
            RHIBufferViewInfo info;
            info.buffer = const_cast<IRHIBuffer*>(this);
            info.offset = offset;
            info.size = size;
            return info;
        }

        //=====================================================================
        // 頂点/インデックスバッファ用
        //=====================================================================

        /// 頂点バッファビュー取得
        RHIVertexBufferView GetVertexBufferView(MemoryOffset offset = 0, MemorySize size = 0, uint32 stride = 0) const;

        /// インデックスバッファビュー取得
        RHIIndexBufferView GetIndexBufferView(ERHIIndexFormat format,
                                              MemoryOffset offset = 0,
                                              MemorySize size = 0) const;

        //=====================================================================
        // Map/Unmap (03-03)
        //=====================================================================

        /// バッファをマップ（CPUアクセス可能にする）
        /// @param mode マップモード
        /// @param offset 開始オフセット（バイト）
        /// @param size サイズ（0=バッファ全体）
        virtual RHIMapResult Map(ERHIMapMode mode, MemoryOffset offset = 0, MemorySize size = 0) = 0;

        /// バッファをアンマップ
        virtual void Unmap(MemoryOffset offset = 0, MemorySize size = 0) = 0;

        /// マップ中か
        virtual bool IsMapped() const = 0;

        //=====================================================================
        // 書き込みヘルパー (03-03)
        //=====================================================================

        /// データを書き込み
        bool WriteData(const void* data, MemorySize size, MemoryOffset offset = 0)
        {
            if (!IsCPUWritable())
                return false;
            RHIMapResult map = Map(ERHIMapMode::WriteDiscard, offset, size);
            if (!map.IsValid())
                return false;
            std::memcpy(map.data, data, size);
            Unmap(offset, size);
            return true;
        }

        template <typename T> bool Write(const T& value, MemoryOffset offset = 0)
        {
            return WriteData(&value, sizeof(T), offset);
        }

        template <typename T> bool WriteArray(const T* data, uint32 count, MemoryOffset offset = 0)
        {
            return WriteData(data, sizeof(T) * count, offset);
        }

        template <typename Container> bool WriteContainer(const Container& container, MemoryOffset offset = 0)
        {
            return WriteData(container.data(), container.size() * sizeof(typename Container::value_type), offset);
        }

        //=====================================================================
        // 読み取りヘルパー (03-03)
        //=====================================================================

        /// データを読み取り
        bool ReadData(void* outData, MemorySize size, MemoryOffset offset = 0)
        {
            if (!IsCPUReadable())
                return false;
            RHIMapResult map = Map(ERHIMapMode::Read, offset, size);
            if (!map.IsValid())
                return false;
            std::memcpy(outData, map.data, size);
            Unmap(offset, size);
            return true;
        }

        template <typename T> bool Read(T& outValue, MemoryOffset offset = 0)
        {
            return ReadData(&outValue, sizeof(T), offset);
        }

        template <typename T> bool ReadArray(T* outData, uint32 count, MemoryOffset offset = 0)
        {
            return ReadData(outData, sizeof(T) * count, offset);
        }

        //=====================================================================
        // IRHIResource マーカー
        //=====================================================================

        bool IsBuffer() const override { return true; }
    };

    /// バッファ参照カウントポインタ
    using RHIBufferRef = TRefCountPtr<IRHIBuffer>;

    //=========================================================================
    // RHIBufferScopeLock (03-03)
    //=========================================================================

    /// バッファスコープロック（RAII）
    class RHI_API RHIBufferScopeLock
    {
    public:
        RHIBufferScopeLock() = default;

        RHIBufferScopeLock(IRHIBuffer* buffer, ERHIMapMode mode, MemoryOffset offset = 0, MemorySize size = 0)
            : m_buffer(buffer), m_offset(offset), m_size(size)
        {
            if (m_buffer)
                m_mapResult = m_buffer->Map(mode, offset, size);
        }

        ~RHIBufferScopeLock() { Unlock(); }

        NS_DISALLOW_COPY(RHIBufferScopeLock);

        RHIBufferScopeLock(RHIBufferScopeLock&& other) noexcept
            : m_buffer(other.m_buffer), m_mapResult(other.m_mapResult), m_offset(other.m_offset), m_size(other.m_size)
        {
            other.m_buffer = nullptr;
            other.m_mapResult = {};
        }

        RHIBufferScopeLock& operator=(RHIBufferScopeLock&& other) noexcept
        {
            if (this != &other)
            {
                Unlock();
                m_buffer = other.m_buffer;
                m_mapResult = other.m_mapResult;
                m_offset = other.m_offset;
                m_size = other.m_size;
                other.m_buffer = nullptr;
                other.m_mapResult = {};
            }
            return *this;
        }

        void Unlock()
        {
            if (m_buffer && m_mapResult.IsValid())
            {
                m_buffer->Unmap(m_offset, m_size);
                m_mapResult = {};
            }
        }

        bool IsValid() const { return m_mapResult.IsValid(); }
        explicit operator bool() const { return IsValid(); }

        void* GetData() const { return m_mapResult.data; }

        template <typename T> T* GetDataAs() const { return m_mapResult.As<T>(); }

        MemorySize GetSize() const { return m_mapResult.size; }

    private:
        IRHIBuffer* m_buffer = nullptr;
        RHIMapResult m_mapResult;
        MemoryOffset m_offset = 0;
        MemorySize m_size = 0;
    };

    //=========================================================================
    // RHITypedBufferLock (03-03)
    //=========================================================================

    /// 型付きバッファスコープロック
    template <typename T> class RHITypedBufferLock
    {
    public:
        RHITypedBufferLock() = default;

        RHITypedBufferLock(IRHIBuffer* buffer, ERHIMapMode mode, uint32 elementOffset = 0, uint32 elementCount = 0)
            : m_lock(buffer, mode, elementOffset * sizeof(T), elementCount > 0 ? elementCount * sizeof(T) : 0),
              m_elementCount(elementCount)
        {
            if (m_elementCount == 0 && buffer)
                m_elementCount = static_cast<uint32>(buffer->GetSize() / sizeof(T));
        }

        bool IsValid() const { return m_lock.IsValid(); }
        explicit operator bool() const { return IsValid(); }

        T* GetData() const { return m_lock.GetDataAs<T>(); }
        uint32 GetCount() const { return m_elementCount; }

        T& operator[](uint32 index)
        {
            RHI_CHECK(index < m_elementCount);
            return GetData()[index];
        }

        const T& operator[](uint32 index) const
        {
            RHI_CHECK(index < m_elementCount);
            return GetData()[index];
        }

        T* begin() { return GetData(); }
        T* end() { return GetData() + m_elementCount; }
        const T* begin() const { return GetData(); }
        const T* end() const { return GetData() + m_elementCount; }

    private:
        RHIBufferScopeLock m_lock;
        uint32 m_elementCount = 0;
    };

    //=========================================================================
    // RHIDynamicBufferUpdater (03-03)
    //=========================================================================

    /// ダイナミックバッファ更新ヘルパー
    class RHI_API RHIDynamicBufferUpdater
    {
    public:
        RHIDynamicBufferUpdater() = default;
        explicit RHIDynamicBufferUpdater(IRHIBuffer* buffer) : m_buffer(buffer) {}

        void SetBuffer(IRHIBuffer* buffer) { m_buffer = buffer; }
        IRHIBuffer* GetBuffer() const { return m_buffer; }

        /// フレーム開始時に呼び出し（WriteDiscardでマップ）
        void* BeginUpdate()
        {
            if (!m_buffer || m_mapped)
                return nullptr;
            m_mapResult = m_buffer->Map(ERHIMapMode::WriteDiscard);
            if (m_mapResult.IsValid())
            {
                m_mapped = true;
                m_writeOffset = 0;
            }
            return m_mapResult.data;
        }

        /// データを連続書き込み
        /// @return 書き込んだ領域のオフセット
        MemoryOffset Write(const void* data, MemorySize size, uint32 alignment = 1)
        {
            if (!m_mapped)
                return 0;
            m_writeOffset = AlignUp(m_writeOffset, static_cast<MemorySize>(alignment));
            if (m_writeOffset + size > m_buffer->GetSize())
                return 0;

            MemoryOffset offset = m_writeOffset;
            std::memcpy(static_cast<uint8*>(m_mapResult.data) + m_writeOffset, data, size);
            m_writeOffset += size;
            return offset;
        }

        template <typename T> MemoryOffset Write(const T& value, uint32 alignment = alignof(T))
        {
            return Write(&value, sizeof(T), alignment);
        }

        /// フレーム終了時に呼び出し
        void EndUpdate()
        {
            if (m_buffer && m_mapped)
            {
                m_buffer->Unmap();
                m_mapped = false;
                m_mapResult = {};
            }
        }

        MemoryOffset GetCurrentOffset() const { return m_writeOffset; }
        MemorySize GetRemainingSize() const { return m_buffer ? m_buffer->GetSize() - m_writeOffset : 0; }

    private:
        IRHIBuffer* m_buffer = nullptr;
        RHIMapResult m_mapResult;
        MemoryOffset m_writeOffset = 0;
        bool m_mapped = false;
    };

} // namespace NS::RHI
