# 03-03: バッファロック/アンロック

## 目的

バッファのCPUアクセス（Map/Unmap）機能を定義する。

## 参照ドキュメント

- 03-02-buffer-interface.md (IRHIBuffer)
- 01-08-enums-buffer.md (ERHIMapMode)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIBuffer.h` (部分

## TODO

### 1. Map/Unmapインターフェース

```cpp
namespace NS::RHI
{
    /// マップ結果
    struct RHI_API RHIMapResult
    {
        /// マップされたメモリポインタ
        void* data = nullptr;

        /// マップされた領域のサイズ
        MemorySize size = 0;

        /// 行ピッチ（テクスチャ用、バッファでは0）。
        uint32 rowPitch = 0;

        /// スライスピッチ（3Dテクスチャ用、バッファでは0）。
        uint32 depthPitch = 0;

        /// 有効か
        bool IsValid() const { return data != nullptr; }

        /// キャスト取得
        template<typename T>
        T* As() const { return static_cast<T*>(data); }
    };

    class IRHIBuffer
    {
    public:
        //=====================================================================
        // Map/Unmap
        //=====================================================================

        /// バッファをマップ（CPUアクセス可能にする）。
        /// @param mode マップモード
        /// @param offset 開始オフセット（バイト）
        /// @param size サイズ（ = バッファ全体）
        /// @return マップ結果
        /// @note CPUWritable/CPUReadableフラグが必要。
        virtual RHIMapResult Map(
            ERHIMapMode mode,
            MemoryOffset offset = 0,
            MemorySize size = 0) = 0;

        /// バッファをアンマップ
        /// @param offset マップ時と同じオフセット
        /// @param size 書き込んだサイズ（ = 全体）
        virtual void Unmap(
            MemoryOffset offset = 0,
            MemorySize size = 0) = 0;

        /// マップ中か
        virtual bool IsMapped() const = 0;
    };
}
```

- [ ] RHIMapResult 構造体
- [ ] Map / Unmap インターフェース
- [ ] IsMapped

### 2. スコープロック（RAII）。

```cpp
namespace NS::RHI
{
    /// バッファスコープロック
    /// スコープを抜けるときに自動でUnmap
    class RHI_API RHIBufferScopeLock
    {
    public:
        RHIBufferScopeLock() = default;

        RHIBufferScopeLock(
            IRHIBuffer* buffer,
            ERHIMapMode mode,
            MemoryOffset offset = 0,
            MemorySize size = 0)
            : m_buffer(buffer)
            , m_offset(offset)
            , m_size(size)
        {
            if (m_buffer) {
                m_mapResult = m_buffer->Map(mode, offset, size);
            }
        }

        ~RHIBufferScopeLock()
        {
            Unlock();
        }

        NS_DISALLOW_COPY(RHIBufferScopeLock);

    public:
        // ムーブ許可
        RHIBufferScopeLock(RHIBufferScopeLock&& other) noexcept
            : m_buffer(other.m_buffer)
            , m_mapResult(other.m_mapResult)
            , m_offset(other.m_offset)
            , m_size(other.m_size)
        {
            other.m_buffer = nullptr;
            other.m_mapResult = {};
        }

        RHIBufferScopeLock& operator=(RHIBufferScopeLock&& other) noexcept
        {
            if (this != &other) {
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

        /// 明示的なアンロック
        void Unlock()
        {
            if (m_buffer && m_mapResult.IsValid()) {
                m_buffer->Unmap(m_offset, m_size);
                m_mapResult = {};
            }
        }

        /// 有効か
        bool IsValid() const { return m_mapResult.IsValid(); }
        explicit operator bool() const { return IsValid(); }

        /// データ取得
        void* GetData() const { return m_mapResult.data; }

        template<typename T>
        T* GetDataAs() const { return m_mapResult.As<T>(); }

        /// サイズ取得
        MemorySize GetSize() const { return m_mapResult.size; }

    private:
        IRHIBuffer* m_buffer = nullptr;
        RHIMapResult m_mapResult;
        MemoryOffset m_offset = 0;
        MemorySize m_size = 0;
    };
}
```

- [ ] RHIBufferScopeLock RAII クラス
- [ ] ムーブセマンティクス

### 3. 型付きスコープロック

```cpp
namespace NS::RHI
{
    /// 型付きバッファスコープロック
    template<typename T>
    class RHITypedBufferLock
    {
    public:
        RHITypedBufferLock() = default;

        RHITypedBufferLock(
            IRHIBuffer* buffer,
            ERHIMapMode mode,
            uint32 elementOffset = 0,
            uint32 elementCount = 0)
            : m_lock(buffer, mode,
                     elementOffset * sizeof(T),
                     elementCount > 0 ? elementCount * sizeof(T) : 0)
            , m_elementCount(elementCount)
        {
            if (m_elementCount == 0 && buffer) {
                m_elementCount = static_cast<uint32>(buffer->GetSize() / sizeof(T));
            }
        }

        /// 有効か
        bool IsValid() const { return m_lock.IsValid(); }
        explicit operator bool() const { return IsValid(); }

        /// 要素へのポインタ取得
        T* GetData() const { return m_lock.GetDataAs<T>(); }

        /// 要素数取得
        uint32 GetCount() const { return m_elementCount; }

        /// 配列アクセス
        T& operator[](uint32 index) {
            RHI_CHECK(index < m_elementCount);
            return GetData()[index];
        }

        const T& operator[](uint32 index) const {
            RHI_CHECK(index < m_elementCount);
            return GetData()[index];
        }

        /// イテレータ
        T* begin() { return GetData(); }
        T* end() { return GetData() + m_elementCount; }
        const T* begin() const { return GetData(); }
        const T* end() const { return GetData() + m_elementCount; }

    private:
        RHIBufferScopeLock m_lock;
        uint32 m_elementCount = 0;
    };
}
```

- [ ] RHITypedBufferLock テンプレート
- [ ] 配列アクセス演算子
- [ ] イテレータサポート

### 4. 書き込みヘルパー

```cpp
namespace NS::RHI
{
    class IRHIBuffer
    {
    public:
        //=====================================================================
        // 書き込みヘルパー
        //=====================================================================

        /// データを書き込み
        /// @param data 書き込むデータ
        /// @param size サイズ（バイト）
        /// @param offset 書き込み先オフセット
        /// @return 成功したか
        bool WriteData(const void* data, MemorySize size, MemoryOffset offset = 0)
        {
            if (!IsCPUWritable()) return false;

            RHIMapResult map = Map(ERHIMapMode::WriteDiscard, offset, size);
            if (!map.IsValid()) return false;

            std::memcpy(map.data, data, size);
            Unmap(offset, size);
            return true;
        }

        /// 型付きデータを書き込み
        template<typename T>
        bool Write(const T& value, MemoryOffset offset = 0)
        {
            return WriteData(&value, sizeof(T), offset);
        }

        /// 配列を書き込み
        template<typename T>
        bool WriteArray(const T* data, uint32 count, MemoryOffset offset = 0)
        {
            return WriteData(data, sizeof(T) * count, offset);
        }

        /// コンテンツを書き込み
        template<typename Container>
        bool WriteContainer(const Container& container, MemoryOffset offset = 0)
        {
            return WriteData(
                container.data(),
                container.size() * sizeof(typename Container::value_type),
                offset);
        }
    };
}
```

- [ ] WriteData
- [ ] Write / WriteArray / WriteContainer

### 5. 読み取りヘルパー

```cpp
namespace NS::RHI
{
    class IRHIBuffer
    {
    public:
        //=====================================================================
        // 読み取りヘルパー
        //=====================================================================

        /// データを読み取り
        /// @param outData 読み取り先
        /// @param size サイズ（バイト）
        /// @param offset 読み取り元のオフセット
        /// @return 成功したか
        bool ReadData(void* outData, MemorySize size, MemoryOffset offset = 0)
        {
            if (!IsCPUReadable()) return false;

            RHIMapResult map = Map(ERHIMapMode::Read, offset, size);
            if (!map.IsValid()) return false;

            std::memcpy(outData, map.data, size);
            Unmap(offset, size);
            return true;
        }

        /// 型付きデータを読み取り
        template<typename T>
        bool Read(T& outValue, MemoryOffset offset = 0)
        {
            return ReadData(&outValue, sizeof(T), offset);
        }

        /// 配列を読み取り
        template<typename T>
        bool ReadArray(T* outData, uint32 count, MemoryOffset offset = 0)
        {
            return ReadData(outData, sizeof(T) * count, offset);
        }
    };
}
```

- [ ] ReadData
- [ ] Read / ReadArray

### 6. ダイナミックバッファ更新パターン

```cpp
namespace NS::RHI
{
    /// ダイナミックバッファ更新ヘルパー
    /// 毎フレーム更新するバッファ向けの最適化パターン
    class RHI_API RHIDynamicBufferUpdater
    {
    public:
        RHIDynamicBufferUpdater() = default;

        /// コンストラクタ
        /// @param buffer 更新対象バッファの（Dynamicフラグ必要）
        explicit RHIDynamicBufferUpdater(IRHIBuffer* buffer)
            : m_buffer(buffer)
        {
        }

        /// バッファ設定
        void SetBuffer(IRHIBuffer* buffer) { m_buffer = buffer; }

        /// バッファ取得
        IRHIBuffer* GetBuffer() const { return m_buffer; }

        /// フレーム開始時に呼び出す
        /// バッファ全体をマップ（WriteDiscard）。
        void* BeginUpdate()
        {
            if (!m_buffer || m_mapped) return nullptr;

            m_mapResult = m_buffer->Map(ERHIMapMode::WriteDiscard);
            if (m_mapResult.IsValid()) {
                m_mapped = true;
                m_writeOffset = 0;
            }
            return m_mapResult.data;
        }

        /// データを書き込み（連続書き込み）。
        /// @return 書き込んだ領域のオフセット
        MemoryOffset Write(const void* data, MemorySize size, uint32 alignment = 1)
        {
            if (!m_mapped) return 0;

            // アライメント調整
            m_writeOffset = AlignUp(m_writeOffset, alignment);

            if (m_writeOffset + size > m_buffer->GetSize()) {
                return 0; // オーバーフロー
            }

            MemoryOffset offset = m_writeOffset;
            std::memcpy(
                static_cast<uint8*>(m_mapResult.data) + m_writeOffset,
                data,
                size);
            m_writeOffset += size;

            return offset;
        }

        /// 型付き書き込み
        template<typename T>
        MemoryOffset Write(const T& value, uint32 alignment = alignof(T))
        {
            return Write(&value, sizeof(T), alignment);
        }

        /// フレーム終了時に呼び出す
        void EndUpdate()
        {
            if (m_buffer && m_mapped) {
                m_buffer->Unmap();
                m_mapped = false;
                m_mapResult = {};
            }
        }

        /// 現在の書き込みオフセット取得
        MemoryOffset GetCurrentOffset() const { return m_writeOffset; }

        /// 残り容量取得
        MemorySize GetRemainingSize() const {
            return m_buffer ? m_buffer->GetSize() - m_writeOffset : 0;
        }

    private:
        IRHIBuffer* m_buffer = nullptr;
        RHIMapResult m_mapResult;
        MemoryOffset m_writeOffset = 0;
        bool m_mapped = false;
    };
}
```

- [ ] RHIDynamicBufferUpdater クラス
- [ ] BeginUpdate / EndUpdate パターン
- [ ] 連続書き込みサポート

## 検証方法

- [ ] Map/Unmapの対称性
- [ ] スコープロテクトのRAII動作
- [ ] 型付きアクセスの正確性
- [ ] ダイナミック更新パターンの効率的
