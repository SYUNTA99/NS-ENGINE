# 03-04: 構造化バッファ

## 目的

構造化バッファ・バイトアドレスバッファの高レベルラップーを定義する。

## 参照ドキュメント

- 03-02-buffer-interface.md (IRHIBuffer)
- 03-03-buffer-lock.md (RHITypedBufferLock)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIStructuredBuffer.h`

## TODO

### 1. RHIStructuredBuffer: 型付きラッパー

```cpp
#pragma once

#include "IRHIBuffer.h"
#include "RHIRefCountPtr.h"

namespace NS::RHI
{
    /// 型付き構造化バッファラッパー
    /// StructuredBuffer<T> / RWStructuredBuffer<T> に対応
    template<typename T>
    class RHIStructuredBuffer
    {
    public:
        using ElementType = T;

        RHIStructuredBuffer() = default;

        /// バッファから構築
        explicit RHIStructuredBuffer(RHIBufferRef buffer)
            : m_buffer(std::move(buffer))
        {
            RHI_CHECK(!m_buffer || m_buffer->GetStride() == sizeof(T));
        }

        /// 作成
        /// @param device デバイス
        /// @param elementCount 要素数
        /// @param usage 追加使用フラグ
        /// @param debugName デバッグ名
        static RHIStructuredBuffer Create(
            IRHIDevice* device,
            uint32 elementCount,
            ERHIBufferUsage additionalUsage = ERHIBufferUsage::None,
            const char* debugName = nullptr);

        /// RWStructuredBuffer用に作成
        static RHIStructuredBuffer CreateRW(
            IRHIDevice* device,
            uint32 elementCount,
            const char* debugName = nullptr);

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 有効か
        bool IsValid() const { return m_buffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        /// 要素数取得
        uint32 GetCount() const {
            return m_buffer ? m_buffer->GetElementCount() : 0;
        }

        /// サイズ取得（バイト）
        MemorySize GetSize() const {
            return m_buffer ? m_buffer->GetSize() : 0;
        }

        /// ストライド取得
        static constexpr uint32 GetStride() { return sizeof(T); }

        /// 内のバッファ取得
        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        RHIBufferRef GetBufferRef() const { return m_buffer; }

        /// GPUアドレス取得
        uint64 GetGPUVirtualAddress() const {
            return m_buffer ? m_buffer->GetGPUVirtualAddress() : 0;
        }

        //=====================================================================
        // アクセス
        //=====================================================================

        /// 書き込みロックを取得
        RHITypedBufferLock<T> Lock(ERHIMapMode mode) {
            return RHITypedBufferLock<T>(m_buffer.Get(), mode);
        }

        /// 読み取りロックを取得
        RHITypedBufferLock<const T> LockRead() {
            return RHITypedBufferLock<const T>(m_buffer.Get(), ERHIMapMode::Read);
        }

        /// 全要素を書き込み
        bool WriteAll(const T* data, uint32 count) {
            if (!m_buffer || count > GetCount()) return false;
            return m_buffer->WriteArray(data, count);
        }

        /// 単一要素を書き込み
        bool WriteAt(uint32 index, const T& value) {
            if (!m_buffer || index >= GetCount()) return false;
            return m_buffer->Write(value, index * sizeof(T));
        }

        /// 単一要素を読み取り
        bool ReadAt(uint32 index, T& outValue) {
            if (!m_buffer || index >= GetCount()) return false;
            return m_buffer->Read(outValue, index * sizeof(T));
        }

    private:
        RHIBufferRef m_buffer;
    };
}
```

- [ ] RHIStructuredBuffer テンプレートクラス
- [ ] Create / CreateRW ファクトリ
- [ ] 型付きアクセス

### 2. Append/Consumeバッファのカウンターサポートppend/Consumeバッファ

```cpp
namespace NS::RHI
{
    /// Append/Consumeバッファ用カウンター付き構造化バッファ
    template<typename T>
    class RHIAppendBuffer
    {
    public:
        using ElementType = T;

        RHIAppendBuffer() = default;

        /// 作成
        static RHIAppendBuffer Create(
            IRHIDevice* device,
            uint32 maxElementCount,
            const char* debugName = nullptr);

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        bool IsValid() const { return m_buffer != nullptr && m_counterBuffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        uint32 GetMaxCount() const {
            return m_buffer ? m_buffer->GetElementCount() : 0;
        }

        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        IRHIBuffer* GetCounterBuffer() const { return m_counterBuffer.Get(); }

        //=====================================================================
        // カウンター操作
        //=====================================================================

        /// カウンターをリセット
        void ResetCounter(IRHICommandContext* context);

        /// カウンター値を読み取り（ステージング経由）。
        /// @note GPU→CPU転送を伴いや延あり
        uint32 ReadCounterValue();

        /// カウンターバッファのGPUアドレス取得
        /// UAVカウンター初期化用
        uint64 GetCounterGPUAddress() const {
            return m_counterBuffer ? m_counterBuffer->GetGPUVirtualAddress() : 0;
        }

    private:
        RHIBufferRef m_buffer;
        RHIBufferRef m_counterBuffer;
    };
}
```

- [ ] RHIAppendBuffer テンプレート
- [ ] カウンターバッファ管理
- [ ] カウンター操作

### 3. バイトアドレスバッファ

```cpp
namespace NS::RHI
{
    /// バイトアドレスバッファラッパー
    /// ByteAddressBuffer / RWByteAddressBuffer に対応
    class RHI_API RHIByteAddressBuffer
    {
    public:
        RHIByteAddressBuffer() = default;

        explicit RHIByteAddressBuffer(RHIBufferRef buffer)
            : m_buffer(std::move(buffer))
        {
        }

        /// 作成
        static RHIByteAddressBuffer Create(
            IRHIDevice* device,
            MemorySize size,
            ERHIBufferUsage additionalUsage = ERHIBufferUsage::None,
            const char* debugName = nullptr);

        /// RWByteAddressBuffer用に作成
        static RHIByteAddressBuffer CreateRW(
            IRHIDevice* device,
            MemorySize size,
            const char* debugName = nullptr);

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        bool IsValid() const { return m_buffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        MemorySize GetSize() const {
            return m_buffer ? m_buffer->GetSize() : 0;
        }

        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        RHIBufferRef GetBufferRef() const { return m_buffer; }

        uint64 GetGPUVirtualAddress() const {
            return m_buffer ? m_buffer->GetGPUVirtualAddress() : 0;
        }

        //=====================================================================
        // 型付きアクセス
        //=====================================================================

        /// 指定オフセットから型付き読み取り
        template<typename T>
        bool Load(MemoryOffset byteOffset, T& outValue) {
            if (!m_buffer) return false;
            return m_buffer->Read(outValue, byteOffset);
        }

        /// 指定オフセットに型付き書き込み
        template<typename T>
        bool Store(MemoryOffset byteOffset, const T& value) {
            if (!m_buffer) return false;
            return m_buffer->Write(value, byteOffset);
        }

        /// uint配列として読み取り
        bool Load4(MemoryOffset byteOffset, uint32 (&outValues)[4]) {
            return Load(byteOffset, outValues);
        }

        /// uint配列として書き込み
        bool Store4(MemoryOffset byteOffset, const uint32 (&values)[4]) {
            return Store(byteOffset, values);
        }

    private:
        RHIBufferRef m_buffer;
    };
}
```

- [ ] RHIByteAddressBuffer クラス
- [ ] Create / CreateRW
- [ ] Load / Store メソッド

### 4. インダイレクト引数バッファ

```cpp
namespace NS::RHI
{
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

    /// インダイレクト引数バッファ
    class RHI_API RHIIndirectArgsBuffer
    {
    public:
        RHIIndirectArgsBuffer() = default;

        explicit RHIIndirectArgsBuffer(RHIBufferRef buffer)
            : m_buffer(std::move(buffer))
        {
        }

        /// DrawIndirect用に作成
        static RHIIndirectArgsBuffer CreateForDraw(
            IRHIDevice* device,
            uint32 maxDrawCount,
            const char* debugName = nullptr);

        /// DrawIndexedIndirect用に作成
        static RHIIndirectArgsBuffer CreateForDrawIndexed(
            IRHIDevice* device,
            uint32 maxDrawCount,
            const char* debugName = nullptr);

        /// DispatchIndirect用に作成
        static RHIIndirectArgsBuffer CreateForDispatch(
            IRHIDevice* device,
            uint32 maxDispatchCount,
            const char* debugName = nullptr);

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        bool IsValid() const { return m_buffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        RHIBufferRef GetBufferRef() const { return m_buffer; }

        uint64 GetGPUVirtualAddress() const {
            return m_buffer ? m_buffer->GetGPUVirtualAddress() : 0;
        }

        //=====================================================================
        // 引数設定
        //=====================================================================

        /// Draw引数を設定
        bool SetDrawArgs(uint32 index, const RHIDrawIndirectArgs& args) {
            if (!m_buffer) return false;
            return m_buffer->Write(args, index * sizeof(RHIDrawIndirectArgs));
        }

        /// DrawIndexed引数を設定
        bool SetDrawIndexedArgs(uint32 index, const RHIDrawIndexedIndirectArgs& args) {
            if (!m_buffer) return false;
            return m_buffer->Write(args, index * sizeof(RHIDrawIndexedIndirectArgs));
        }

        /// Dispatch引数を設定
        bool SetDispatchArgs(uint32 index, const RHIDispatchIndirectArgs& args) {
            if (!m_buffer) return false;
            return m_buffer->Write(args, index * sizeof(RHIDispatchIndirectArgs));
        }

        /// 指定インデックスのオフセット取得（Draw用）。
        MemoryOffset GetDrawArgsOffset(uint32 index) const {
            return index * sizeof(RHIDrawIndirectArgs);
        }

        /// 指定インデックスのオフセット取得（DrawIndexed用）。
        MemoryOffset GetDrawIndexedArgsOffset(uint32 index) const {
            return index * sizeof(RHIDrawIndexedIndirectArgs);
        }

        /// 指定インデックスのオフセット取得（Dispatch用）。
        MemoryOffset GetDispatchArgsOffset(uint32 index) const {
            return index * sizeof(RHIDispatchIndirectArgs);
        }

    private:
        RHIBufferRef m_buffer;
    };
}
```

- [ ] RHIDrawIndirectArgs / RHIDrawIndexedIndirectArgs / RHIDispatchIndirectArgs
- [ ] RHIIndirectArgsBuffer クラス
- [ ] 引数設定メソッド

### 5. 定数バッファラッパー

```cpp
namespace NS::RHI
{
    /// 型付き定数バッファラッパー
    template<typename T>
    class RHIConstantBuffer
    {
    public:
        using DataType = T;

        static_assert(sizeof(T) <= 65536, "Constant buffer too large (max 64KB)");

        RHIConstantBuffer() = default;

        /// 作成
        static RHIConstantBuffer Create(
            IRHIDevice* device,
            const char* debugName = nullptr);

        /// 動的定数バッファとして作成
        static RHIConstantBuffer CreateDynamic(
            IRHIDevice* device,
            const char* debugName = nullptr);

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        bool IsValid() const { return m_buffer != nullptr; }
        explicit operator bool() const { return IsValid(); }

        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }
        RHIBufferRef GetBufferRef() const { return m_buffer; }

        uint64 GetGPUVirtualAddress() const {
            return m_buffer ? m_buffer->GetGPUVirtualAddress() : 0;
        }

        /// アライメント済みサイズ取得
        static constexpr MemorySize GetAlignedSize() {
            return GetConstantBufferSize<T>();
        }

        //=====================================================================
        // 更新
        //=====================================================================

        /// データを更新
        bool Update(const T& data) {
            if (!m_buffer) return false;
            return m_buffer->Write(data);
        }

        /// 部分更新
        template<typename MemberType>
        bool UpdateMember(MemberType T::* member, const MemberType& value) {
            if (!m_buffer) return false;
            MemoryOffset offset = reinterpret_cast<MemoryOffset>(
                &(static_cast<T*>(nullptr)->*member));
            return m_buffer->Write(value, offset);
        }

    private:
        RHIBufferRef m_buffer;
    };
}
```

- [ ] RHIConstantBuffer テンプレート
- [ ] Update / UpdateMember

## 検証方法

- [ ] 構造化バッファのストライド整合性
- [ ] Append/Consumeカウンターの動作
- [ ] インダイレクト引数のレイアウト
- [ ] 定数バッファのアライメント
