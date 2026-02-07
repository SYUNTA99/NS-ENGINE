# 04-04: テクスチャデータアップロード

## 目的

テクスチャへのデータ転送の更新機能を定義する。

## 参照ドキュメント

- 04-02-texture-interface.md (IRHITexture)
- 04-03-texture-subresource.md (RHISubresourceLayout)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHITexture.h` (部分

## TODO

### 1. テクスチャ初期データ

```cpp
namespace NS::RHI
{
    /// テクスチャサブリソース初期データ
    struct RHI_API RHITextureSubresourceData
    {
        /// データポインタ
        const void* data = nullptr;

        /// 行ピッチ（バイト）
        /// 1行のデータサイズ
        uint32 rowPitch = 0;

        /// スライスピッチ（バイト）
        /// 3Dテクスチャの1スライス分、または2D配列の1要素分
        uint32 depthPitch = 0;
    };

    /// テクスチャ初期化データ
    struct RHI_API RHITextureInitData
    {
        /// サブリソースデータ配列
        /// インデックス = mip + arraySlice * mipLevels
        const RHITextureSubresourceData* subresources = nullptr;

        /// サブリソース数
        uint32 subresourceCount = 0;

        /// 単一サブリソースで初期化
        static RHITextureInitData Single(const RHITextureSubresourceData& data) {
            RHITextureInitData init;
            init.subresources = &data;
            init.subresourceCount = 1;
            return init;
        }
    };

    /// 作成と同時に初期化するテクスチャDesc
    struct RHI_API RHITextureCreateInfo
    {
        RHITextureDesc desc;
        RHITextureInitData initData;
    };
}
```

- [ ] RHITextureSubresourceData 構造体
- [ ] RHITextureInitData 構造体
- [ ] RHITextureCreateInfo 構造体

### 2. Map/Unmapインターフェース

```cpp
namespace NS::RHI
{
    /// テクスチャマップ結果
    struct RHI_API RHITextureMapResult
    {
        /// マップされたデータポインタ
        void* data = nullptr;

        /// 行ピッチ（バイト）
        uint32 rowPitch = 0;

        /// 深度ピッチ（バイト）
        uint32 depthPitch = 0;

        /// マップサイズ
        MemorySize size = 0;

        /// 有効か
        bool IsValid() const { return data != nullptr; }

        /// キャスト取得
        template<typename T>
        T* As() const { return static_cast<T*>(data); }

        /// 指定行へのポインタ取得
        void* GetRowPointer(uint32 row) const {
            return static_cast<uint8*>(data) + row * rowPitch;
        }

        /// 指定スライスへのポインタ取得（D/配列）。
        void* GetSlicePointer(uint32 slice) const {
            return static_cast<uint8*>(data) + slice * depthPitch;
        }
    };

    class IRHITexture
    {
    public:
        //=====================================================================
        // Map/Unmap
        //=====================================================================

        /// テクスチャサブリソースをマップ。
        /// @param mipLevel MIPレベル
        /// @param arraySlice 配列スライス
        /// @param mode マップモード
        /// @return マップ結果
        /// @note CPUWritable/CPUReadableフラグ、またはリニアレイアウトが必要。
        virtual RHITextureMapResult Map(
            uint32 mipLevel,
            uint32 arraySlice,
            ERHIMapMode mode) = 0;

        /// テクスチャをアンマップ
        /// @param mipLevel MIPレベル
        /// @param arraySlice 配列スライス
        virtual void Unmap(uint32 mipLevel, uint32 arraySlice) = 0;

        /// マップ可能か
        bool CanMap() const {
            return IsCPUReadable() || IsCPUWritable();
        }
    };
}
```

- [ ] RHITextureMapResult 構造体
- [ ] Map / Unmap インターフェース

### 3. テクスチャスコープロック

```cpp
namespace NS::RHI
{
    /// テクスチャスコープロック（RAII）。
    class RHI_API RHITextureScopeLock
    {
    public:
        RHITextureScopeLock() = default;

        RHITextureScopeLock(
            IRHITexture* texture,
            uint32 mipLevel,
            uint32 arraySlice,
            ERHIMapMode mode)
            : m_texture(texture)
            , m_mipLevel(mipLevel)
            , m_arraySlice(arraySlice)
        {
            if (m_texture) {
                m_mapResult = m_texture->Map(mipLevel, arraySlice, mode);
            }
        }

        ~RHITextureScopeLock() {
            Unlock();
        }

        NS_DISALLOW_COPY(RHITextureScopeLock);

    public:
        // ムーブ許可
        RHITextureScopeLock(RHITextureScopeLock&& other) noexcept
            : m_texture(other.m_texture)
            , m_mapResult(other.m_mapResult)
            , m_mipLevel(other.m_mipLevel)
            , m_arraySlice(other.m_arraySlice)
        {
            other.m_texture = nullptr;
            other.m_mapResult = {};
        }

        RHITextureScopeLock& operator=(RHITextureScopeLock&& other) noexcept {
            if (this != &other) {
                Unlock();
                m_texture = other.m_texture;
                m_mapResult = other.m_mapResult;
                m_mipLevel = other.m_mipLevel;
                m_arraySlice = other.m_arraySlice;
                other.m_texture = nullptr;
                other.m_mapResult = {};
            }
            return *this;
        }

        void Unlock() {
            if (m_texture && m_mapResult.IsValid()) {
                m_texture->Unmap(m_mipLevel, m_arraySlice);
                m_mapResult = {};
            }
        }

        bool IsValid() const { return m_mapResult.IsValid(); }
        explicit operator bool() const { return IsValid(); }

        void* GetData() const { return m_mapResult.data; }
        uint32 GetRowPitch() const { return m_mapResult.rowPitch; }
        uint32 GetDepthPitch() const { return m_mapResult.depthPitch; }

        void* GetRowPointer(uint32 row) const {
            return m_mapResult.GetRowPointer(row);
        }

    private:
        IRHITexture* m_texture = nullptr;
        RHITextureMapResult m_mapResult;
        uint32 m_mipLevel = 0;
        uint32 m_arraySlice = 0;
    };
}
```

- [ ] RHITextureScopeLock RAII クラス

### 4. アップロードヘルパー。

```cpp
namespace NS::RHI
{
    /// テクスチャアップロードヘルパー。
    /// ステージングバッファ経由でGPUテクスチャに転送
    class RHI_API RHITextureUploader
    {
    public:
        RHITextureUploader() = default;

        /// コンストラクタ
        /// @param device デバイス
        /// @param commandContext コマンドコンテキスト
        RHITextureUploader(IRHIDevice* device, IRHICommandContext* context);

        /// 2Dテクスチャへアップロード
        /// @param dst 転送のテクスチャ
        /// @param mipLevel MIPレベル
        /// @param srcData ソースデータ
        /// @param srcRowPitch ソース行ピッチ
        /// @return 成功したか
        bool Upload2D(
            IRHITexture* dst,
            uint32 mipLevel,
            const void* srcData,
            uint32 srcRowPitch);

        /// 配列スライスへアップロード
        bool Upload2DArray(
            IRHITexture* dst,
            uint32 mipLevel,
            uint32 arraySlice,
            const void* srcData,
            uint32 srcRowPitch);

        /// 3Dテクスチャへアップロード
        bool Upload3D(
            IRHITexture* dst,
            uint32 mipLevel,
            const void* srcData,
            uint32 srcRowPitch,
            uint32 srcDepthPitch);

        /// キューブマップの面へアップロード
        bool UploadCubeFace(
            IRHITexture* dst,
            ERHICubeFace face,
            uint32 mipLevel,
            const void* srcData,
            uint32 srcRowPitch);

        /// 部分領域へアップロード
        bool UploadRegion(
            IRHITexture* dst,
            uint32 mipLevel,
            uint32 arraySlice,
            const RHIBox& dstRegion,
            const void* srcData,
            uint32 srcRowPitch,
            uint32 srcDepthPitch);

        /// 全MIPレベルを自動生成（GenerateMipsフラグ必要）
        void GGenerateMips(IRHITexture* texture);

    private:
        IRHIDevice* m_device = nullptr;
        IRHICommandContext* m_context = nullptr;
        RHIBufferRef m_stagingBuffer;
    };
}
```

- [ ] RHITextureUploader クラス
- [ ] Upload2D / Upload3D
- [ ] UploadCubeFace
- [ ] UploadRegion
- [ ] GGenerateMips

### 5. リードバックヘルパー

```cpp
namespace NS::RHI
{
    /// テクスチャリードバックヘルパー
    /// GPUテクスチャからCPUへ読み取り
    class RHI_API RHITextureReadback
    {
    public:
        RHITextureReadback() = default;

        /// コンストラクタ
        RHITextureReadback(IRHIDevice* device, IRHICommandContext* context);

        /// 2Dテクスチャを読み取り
        /// @param src ソーステクスチャ
        /// @param mipLevel MIPレベル
        /// @param dstData 出力バッファ
        /// @param dstRowPitch 出力行ピッチ
        /// @return 成功したか
        /// @note この操作はGPU同期を含むため低速
        bool Read2D(
            IRHITexture* src,
            uint32 mipLevel,
            void* dstData,
            uint32 dstRowPitch);

        /// 非同期リードバック開始
        /// @return リードバックID
        uint64 BeginAsyncRead(
            IRHITexture* src,
            uint32 mipLevel,
            uint32 arraySlice = 0);

        /// 非同期リードバック完了確認
        bool IsReadComplete(uint64 readbackId);

        /// 非同期リードバック結果取得
        /// @param readbackId リードバックID
        /// @param dstData 出力バッファ
        /// @param dstRowPitch 出力行ピッチ
        bool GetReadResult(
            uint64 readbackId,
            void* dstData,
            uint32 dstRowPitch);

    private:
        IRHIDevice* m_device = nullptr;
        IRHICommandContext* m_context = nullptr;
        // 内のリードバック管理
    };
}
```

- [ ] RHITextureReadback クラス
- [ ] Read2D（同期版）。
- [ ] BeginAsyncRead / IsReadComplete / GetReadResult（非同期版）

### 6. テクスチャコピー補助

```cpp
namespace NS::RHI
{
    /// テクスチャコピー記述
    struct RHI_API RHITextureCopyDesc
    {
        /// ソーステクスチャ
        IRHITexture* srcTexture = nullptr;

        /// ソースサブリソース
        uint32 srcMipLevel = 0;
        uint32 srcArraySlice = 0;

        /// ソースオフセット
        Offset3D srcOffset = {0, 0, 0};

        /// デスティネーションテクスチャ
        IRHITexture* dstTexture = nullptr;

        /// デスティネーションサブリソース
        uint32 dstMipLevel = 0;
        uint32 dstArraySlice = 0;

        /// デスティネーションオフセット
        Offset3D dstOffset = {0, 0, 0};

        /// コピーサイズ（ = ソースMIPの全体）
        Extent3D extent = {0, 0, 0};
    };

    /// テクスチャコピーヘルパー
    /// コピー操作の検証と実行
    class RHI_API RHITextureCopyHelper
    {
    public:
        /// コピーが有効か検証
        static bool Validate(const RHITextureCopyDesc& desc);

        /// コピー後のバリアを自動挿入するか
        static void CopyWithBarriers(
            IRHICommandContext* context,
            const RHITextureCopyDesc& desc);

        /// フォーマット変換コピー（互換フォーマット間）。
        static bool CopyWithFormatConversion(
            IRHICommandContext* context,
            const RHITextureCopyDesc& desc);
    };
}
```

- [ ] RHITextureCopyDesc 構造体
- [ ] RHITextureCopyHelper クラス
- [ ] Validate / CopyWithBarriers

## 検証方法

- [ ] Map/Unmapの対称性
- [ ] スコープロテクトのRAII動作
- [ ] アップロードのリードバックの正確性
- [ ] 非同期リードバックの同期
- [ ] コピー検証の網羅性
