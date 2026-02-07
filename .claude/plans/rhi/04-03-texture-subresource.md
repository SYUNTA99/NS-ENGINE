# 04-03: テクスチャサブリソース

## 目的

テクスチャのサブリソース（MIPレベル、配列スライス（アクセス機能を定義する。

## 参照ドキュメント

- 04-02-texture-interface.md (IRHITexture)
- 01-09-enums-texture.md (ERHITextureDimension)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHITexture.h` (部分

## TODO

### 1. サブリソースインデックス計算

```cpp
namespace NS::RHI
{
    /// サブリソースインデックス
    /// D3D12形式: mipSlice + arraySlice * mipLevels + planeSlice * mipLevels * arraySize
    using SubresourceIndex = uint32;

    /// 無効サブリソース
    constexpr SubresourceIndex kInvalidSubresource = ~0u;

    /// サブリソースインデックス計算
    /// @param mipSlice MIPレベル（ = 最大解像度）。
    /// @param arraySlice 配列スライス
    /// @param planeSlice 平面スライス（デプス/ステンシル分離など）。
    /// @param mipLevels 総MIPレベル数
    /// @param arraySize 配列サイズ
    inline constexpr SubresourceIndex CalculateSubresource(
        uint32 mipSlice,
        uint32 arraySlice,
        uint32 planeSlice,
        uint32 mipLevels,
        uint32 arraySize)
    {
        return mipSlice + (arraySlice * mipLevels) + (planeSlice * mipLevels * arraySize);
    }

    /// サブリソースインデックスからMIPレベル取得
    inline constexpr uint32 GetMipFromSubresource(
        SubresourceIndex subresource,
        uint32 mipLevels)
    {
        return subresource % mipLevels;
    }

    /// サブリソースインデックスから配列スライス取得
    inline constexpr uint32 GetArraySliceFromSubresource(
        SubresourceIndex subresource,
        uint32 mipLevels,
        uint32 arraySize)
    {
        return (subresource / mipLevels) % arraySize;
    }

    /// サブリソースインデックスから平面スライス取得
    inline constexpr uint32 GetPlaneFromSubresource(
        SubresourceIndex subresource,
        uint32 mipLevels,
        uint32 arraySize)
    {
        return subresource / (mipLevels * arraySize);
    }
}
```

- [ ] SubresourceIndex 型
- [ ] CalculateSubresource
- [ ] Get~FromSubresource ユーティリティ

### 2. サブリソース範囲

```cpp
namespace NS::RHI
{
    /// サブリソース範囲
    struct RHI_API RHISubresourceRange
    {
        /// 開始MIPレベル
        uint32 baseMipLevel = 0;

        /// MIPレベル数（ = 残り全て）。
        uint32 levelCount = 1;

        /// 開始配列スライス
        uint32 baseArrayLayer = 0;

        /// 配列レイヤー数（ = 残り全て）。
        uint32 layerCount = 1;

        /// 平面スライス（通常0）。
        uint32 planeSlice = 0;

        /// 全サブリソースを含む範囲
        static RHISubresourceRange All() {
            RHISubresourceRange range;
            range.baseMipLevel = 0;
            range.levelCount = 0;     // 全MIP
            range.baseArrayLayer = 0;
            range.layerCount = 0;     // 全配列
            return range;
        }

        /// 単一MIPレベル
        static RHISubresourceRange SingleMip(uint32 mipLevel, uint32 arrayLayer = 0) {
            RHISubresourceRange range;
            range.baseMipLevel = mipLevel;
            range.levelCount = 1;
            range.baseArrayLayer = arrayLayer;
            range.layerCount = 1;
            return range;
        }

        /// 単一配列スライス（全MIP）。
        static RHISubresourceRange SingleArraySlice(uint32 arrayLayer, uint32 mipCount = 0) {
            RHISubresourceRange range;
            range.baseMipLevel = 0;
            range.levelCount = mipCount;
            range.baseArrayLayer = arrayLayer;
            range.layerCount = 1;
            return range;
        }

        /// MIP範囲
        static RHISubresourceRange MipRange(uint32 baseMip, uint32 count) {
            RHISubresourceRange range;
            range.baseMipLevel = baseMip;
            range.levelCount = count;
            range.baseArrayLayer = 0;
            range.layerCount = 0;  // 全配列
            return range;
        }
    };

    /// IRHITexture用サブリソース範囲ヘルパー
    class IRHITexture
    {
    public:
        /// 総サブリソース数取得
        uint32 GetTotalSubresourceCount() const {
            return GetMipLevels() * GetArraySize() * GetPlaneCount();
        }

        /// 平面数取得（デプス/ステンシル分離フォーマットの場合）。
        uint32 GetPlaneCount() const {
            return GetPixelFormatPlaneCount(GetFormat());
        }

        /// サブリソースインデックス計算
        SubresourceIndex GetSubresourceIndex(
            uint32 mipLevel,
            uint32 arraySlice = 0,
            uint32 planeSlice = 0) const
        {
            return CalculateSubresource(
                mipLevel, arraySlice, planeSlice,
                GetMipLevels(), GetArraySize());
        }

        /// 範囲が有効か検証
        bool IsValidSubresourceRange(const RHISubresourceRange& range) const {
            uint32 maxMip = range.baseMipLevel +
                (range.levelCount > 0 ? range.levelCount : GetMipLevels() - range.baseMipLevel);
            uint32 maxArray = range.baseArrayLayer +
                (range.layerCount > 0 ? range.layerCount : GetArraySize() - range.baseArrayLayer);

            return maxMip <= GetMipLevels() && maxArray <= GetArraySize();
        }
    };
}
```

- [ ] RHISubresourceRange 構造体
- [ ] GetTotalSubresourceCount / GetPlaneCount
- [ ] GetSubresourceIndex

### 3. サブリソースデータレイアウト

```cpp
namespace NS::RHI
{
    /// サブリソースデータレイアウト
    struct RHI_API RHISubresourceLayout
    {
        /// データ開始オフセット（バイト）
        MemoryOffset offset = 0;

        /// サブリソース全体サイズ（バイト）
        MemorySize size = 0;

        /// 行ピッチ（バイト）
        uint32 rowPitch = 0;

        /// スライスピッチ（3Dテクスチャ/配列の1要素分のサイズ、バイト）
        uint32 depthPitch = 0;

        /// 幅（ピクセルまたはブロック単位）
        uint32 width = 0;

        /// 高さ（ピクセルまたはブロック単位）
        uint32 height = 0;

        /// 深度
        uint32 depth = 1;
    };

    class IRHITexture
    {
    public:
        //=====================================================================
        // サブリソースレイアウト
        //=====================================================================

        /// サブリソースレイアウト取得
        /// @param mipLevel MIPレベル
        /// @param arraySlice 配列スライス
        /// @return レイアウト情報
        virtual RHISubresourceLayout GetSubresourceLayout(
            uint32 mipLevel,
            uint32 arraySlice = 0) const = 0;

        /// アップロード用に必要なサイズ計算
        MemorySize CalculateUploadSize(const RHISubresourceRange& range) const;

        /// 行数取得（圧縮フォーマット考（。
        uint32 GetRowCount(uint32 mipLevel) const {
            uint32 height = CalculateMipSize(GetHeight(), mipLevel);
            uint32 blockHeight = GetPixelFormatBlockHeight(GetFormat());
            return (height + blockHeight - 1) / blockHeight;
        }
    };
}
```

- [ ] RHISubresourceLayout 構造体
- [ ] GetSubresourceLayout
- [ ] CalculateUploadSize

### 4. キューブマップの面定義

```cpp
namespace NS::RHI
{
    /// キューブマップの面
    enum class ERHICubeFace : uint8
    {
        PositiveX = 0,  // +X (Right)
        NegativeX = 1,  // -X (Left)
        PositiveY = 2,  // +Y (Top)
        NegativeY = 3,  // -Y (Bottom)
        PositiveZ = 4,  // +Z (Front)
        NegativeZ = 5,  // -Z (Back)
    };

    /// キューブマップの面数
    constexpr uint32 kCubeFaceCount = 6;

    /// キューブマップの面名取得
    inline const char* GetCubeFaceName(ERHICubeFace face)
    {
        switch (face) {
            case ERHICubeFace::PositiveX: return "+X";
            case ERHICubeFace::NegativeX: return "-X";
            case ERHICubeFace::PositiveY: return "+Y";
            case ERHICubeFace::NegativeY: return "-Y";
            case ERHICubeFace::PositiveZ: return "+Z";
            case ERHICubeFace::NegativeZ: return "-Z";
            default:                       return "Unknown";
        }
    }

    class IRHITexture
    {
    public:
        /// キューブマップの面のサブリソースインデックス取得
        SubresourceIndex GetCubeFaceSubresource(
            ERHICubeFace face,
            uint32 mipLevel = 0,
            uint32 cubeIndex = 0) const
        {
            RHI_CHECK(IsCube());
            uint32 faceIndex = static_cast<uint32>(face);
            uint32 arraySlice = cubeIndex * kCubeFaceCount + faceIndex;
            return GetSubresourceIndex(mipLevel, arraySlice);
        }
    };
}
```

- [ ] ERHICubeFace 列挙型
- [ ] GetCubeFaceName
- [ ] GetCubeFaceSubresource

### 5. サブリソースイテレータ

```cpp
namespace NS::RHI
{
    /// サブリソース範囲イテレータ
    /// 指定範囲内全サブリソースを走査
    class RHI_API RHISubresourceIterator
    {
    public:
        RHISubresourceIterator(
            const IRHITexture* texture,
            const RHISubresourceRange& range)
            : m_texture(texture)
            , m_range(range)
            , m_currentMip(range.baseMipLevel)
            , m_currentArray(range.baseArrayLayer)
            , m_currentPlane(range.planeSlice)
        {
            // 範囲解決
            m_maxMip = range.baseMipLevel +
                (range.levelCount > 0 ? range.levelCount : texture->GetMipLevels() - range.baseMipLevel);
            m_maxArray = range.baseArrayLayer +
                (range.layerCount > 0 ? range.layerCount : texture->GetArraySize() - range.baseArrayLayer);
        }

        /// 次のサブリソースへ進む
        bool Next() {
            ++m_currentMip;
            if (m_currentMip >= m_maxMip) {
                m_currentMip = m_range.baseMipLevel;
                ++m_currentArray;
                if (m_currentArray >= m_maxArray) {
                    return false;
                }
            }
            return true;
        }

        /// 現在のサブリソースインデックス取得
        SubresourceIndex GetCurrentIndex() const {
            return m_texture->GetSubresourceIndex(m_currentMip, m_currentArray, m_currentPlane);
        }

        /// 現在のMIPレベル取得
        uint32 GetCurrentMip() const { return m_currentMip; }

        /// 現在の配列スライス取得
        uint32 GetCurrentArraySlice() const { return m_currentArray; }

        /// 終了判定
        bool IsEnd() const {
            return m_currentArray >= m_maxArray;
        }

    private:
        const IRHITexture* m_texture;
        RHISubresourceRange m_range;
        uint32 m_currentMip;
        uint32 m_currentArray;
        uint32 m_currentPlane;
        uint32 m_maxMip;
        uint32 m_maxArray;
    };

    /// 範囲内のサブリソースを列挙（コールバック版）
    template<typename Func>
    void ForEachSubresource(
        const IRHITexture* texture,
        const RHISubresourceRange& range,
        Func&& func)
    {
        RHISubresourceIterator it(texture, range);
        do {
            func(it.GetCurrentIndex(), it.GetCurrentMip(), it.GetCurrentArraySlice());
        } while (it.Next());
    }
}
```

- [ ] RHISubresourceIterator クラス
- [ ] ForEachSubresource テンプレート

## 検証方法

- [ ] サブリソースインデックス計算の正確性
- [ ] 範囲検証の正確性
- [ ] キューブマップの面マッピングの正確性
- [ ] イテレータの網羅性
