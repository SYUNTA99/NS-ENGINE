# 07-07: Variable Rate Shading (VRS)

## 目的

画面領域ごとに異なるシェーディングレートを適用するVRSシステムを定義する。

## 参照ドキュメント

- .claude/plans/rhi/docs/RHI_Implementation_Guide_Part11.md (3. Variable Rate Shading)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIVariableRateShading.h`

既存ファイル修正:
- `Source/Engine/RHI/Public/IRHICommandContext.h` (VRSコマンド追加)
- `Source/Engine/RHI/Public/RHIRenderPass.h` (VRSアタッチメント追加)

## TODO

### シェーディングレート列挙型

```cpp
/// 軸ごとのシェーディングレーチ
enum class ERHIVRSAxisRate : uint8 {
    Rate_1X = 0x0,  // フルレート(1ピクセル = 1シェーディング)
    Rate_2X = 0x1,  // 1/2レート(2ピクセル = 1シェーディング)
    Rate_4X = 0x2   // 1/4レート(4ピクセル = 1シェーディング)
};

/// 2Dシェーディングレート(X軸 x Y軸)
enum class ERHIShadingRate : uint8 {
    Rate_1x1 = 0x00,  // 1x1 フルレーチ
    Rate_1x2 = 0x01,  // 1x2
    Rate_2x1 = 0x04,  // 2x1
    Rate_2x2 = 0x05,  // 2x2 (デフォルトの低品質)
    Rate_2x4 = 0x06,  // 2x4 (Large size)
    Rate_4x2 = 0x09,  // 4x2 (Large size)
    Rate_4x4 = 0x0A   // 4x4 最低品質
};

/// シェーディングレートをピクセル数に変換
inline uint32 GetShadingRatePixelCountX(ERHIShadingRate rate) {
    return 1u << ((static_cast<uint8>(rate) >> 2) & 0x3);
}

inline uint32 GetShadingRatePixelCountY(ERHIShadingRate rate) {
    return 1u << (static_cast<uint8>(rate) & 0x3);
}
```

### レートコンバイナ。

```cpp
/// 複数のシェーディングレートソースを組み合わせる方法
enum class ERHIVRSCombiner : uint8 {
    /// 入力をそのまま出力(無視
    Passthrough,

    /// 入力で上書か
    Override,

    /// 最小値を選択(最高品質)
    Min,

    /// 最大値を選択(最低品質)
    Max,

    /// 合計(品質を下げる方合
    Sum
};
```

### VRSイメージタイプ

```cpp
/// VRSイメージデータタイプ
enum class ERHIVRSImageType : uint8 {
    /// VRSイメージ非サポート
    NotSupported,

    /// パレットベース (離散値)
    /// 例: 0=1x1, 1=2x2, 2=4x4
    Palette,

    /// 浮動小数点ベース (連続値)
    /// 例: 1.0=フルレート 0.5=1/2, 0.25=1/4
    Fractional
};
```

### VRSケイパビリティ

```cpp
/// VRSケイパビリティ情報
struct RHIVRSCapabilities {
    /// パイプラインVRSサポート(描画単位でレート設定
    bool supportsPipelineVRS = false;

    /// 大サイズVRSサポート(2x4, 4x2, 4x4)
    bool supportsLargerSizes = false;

    /// イメージベースVRSサポート(スクリーン空間イメージ)
    bool supportsImageVRS = false;

    /// Per-Primitive VRS対応（GS/MSから SV_ShadingRate 設定可能）
    bool supportsPerPrimitiveVRS = false;

    /// 複数なコンバイナの操作サポート(Min/Max/Sum)
    bool supportsComplexCombiners = false;

    /// アレイテクスチャとしてのVRSアタッチメント
    bool supportsArrayTextures = false;

    /// VRSイメージのタイルサイズ
    uint32 imageTileMinWidth = 0;
    uint32 imageTileMinHeight = 0;
    uint32 imageTileMaxWidth = 0;
    uint32 imageTileMaxHeight = 0;

    /// イメージデータタイプ
    ERHIVRSImageType imageType = ERHIVRSImageType::NotSupported;

    /// VRSイメージフォーマット
    EPixelFormat imageFormat = EPixelFormat::Unknown;
};

/// VRSケイパビリティを取得(IRHIDeviceに追加)
virtual const RHIVRSCapabilities& GetVRSCapabilities() const = 0;
```

### VRSイメージ作成

```cpp
/// VRSイメージ記述
struct RHIVRSImageDesc {
    /// ターゲット解像度 (VRSイメージサイズは自動計算
    uint32 targetWidth;
    uint32 targetHeight;

    /// タイルサイズ (0=最大を使用)
    uint32 tileWidth = 0;
    uint32 tileHeight = 0;

    /// デバッグ名
    const char* debugName = nullptr;
};

/// VRSイメージサイズ計算ヘルパー。
inline void CalculateVRSImageSize(
    const RHIVRSCapabilities& caps,
    uint32 targetWidth, uint32 targetHeight,
    uint32& outImageWidth, uint32& outImageHeight,
    uint32 tileWidth = 0, uint32 tileHeight = 0)
{
    uint32 tw = (tileWidth > 0) ? tileWidth : caps.imageTileMaxWidth;
    uint32 th = (tileHeight > 0) ? tileHeight : caps.imageTileMaxHeight;

    outImageWidth = (targetWidth + tw - 1) / tw;
    outImageHeight = (targetHeight + th - 1) / th;
}
```

### コマンドコンテキスト拡張

```cpp
class IRHICommandContext {
    // ... 既存メソッド...

    //=========================================================================
    // Variable Rate Shading
    //=========================================================================

    /// パイプラインシェーディングレート設定
    /// @param rate 基本シェーディングレート
    /// @param combiners [0]=パイプラインとプリミティブの組み合わせ
    ///                  [1]=上記結果とイメージの組み合わせ
    /// @note combiners=nullptrの場合、両方Passthroughとして扱う
    virtual void SetShadingRate(
        ERHIShadingRate rate,
        const ERHIVRSCombiner combiners[2] = nullptr
    ) = 0;

    /// VRSイメージ設定
    /// @param vrsImage VRSイメージテクスチャ (nullptr=無効)
    virtual void SetShadingRateImage(IRHITexture* vrsImage) = 0;
};
```

### レンダーパス拡張

```cpp
/// レンダーパス記述子にVRS追加
struct RHIRenderPassDesc {
    // ... 既存メソッド...

    /// VRSアタッチメント
    struct {
        /// VRSイメージ
        IRHITexture* image = nullptr;

        /// コンバイナの設定
        ERHIVRSCombiner combiner = ERHIVRSCombiner::Passthrough;
    } shadingRate;
};
```

### プラットフォームサポート

```
| API            | Pipeline VRS | Image VRS | Per-Primitive | Large Sizes |
|----------------|:---:|:---:|:---:|:---:|
| D3D12 Tier 1   | ✓ | - | - | - |
| D3D12 Tier 2   | ✓ | ✓ | ✓ | ✓ |
| Vulkan (基本)  | ✓ | ✓ | ✓ | ✓ |
| Metal          | ✓ | ✓ | - | ✓ |
| Xbox Series X  | ✓ | ✓ | ✓ | ✓ |
| PlayStation 5  | ✓ | ✓ | ✓ | ✓ |
```

### 使用パターン

**パイプラインVRS (描画単位:**
```cpp
// 遠距離オブジェクトの低品質でシェーディング
if (distance > farThreshold) {
    context->SetShadingRate(ERHIShadingRate::Rate_2x2, nullptr);
} else {
    context->SetShadingRate(ERHIShadingRate::Rate_1x1, nullptr);
}
context->DrawIndexed(...);
```

**イメージベースVRS (フォビエーション):**
```cpp
// 1. VRSイメージ作成
auto caps = device->GetVRSCapabilities();
uint32 vrsWidth, vrsHeight;
CalculateVRSImageSize(caps, screenWidth, screenHeight, vrsWidth, vrsHeight);

RHITextureDesc vrsDesc;
vrsDesc.width = vrsWidth;
vrsDesc.height = vrsHeight;
vrsDesc.format = caps.imageFormat;
vrsDesc.flags = ERHITextureFlags::UAV | ERHITextureFlags::ShaderResource;

auto vrsImage = device->CreateTexture(vrsDesc);

// 2. VRSイメージを生成(コンピュートシェーダーで)
// 視線中央の1x1、周辺は2x2または4x4
context->SetComputePipeline(vrsGenerationPipeline);
context->SetUAV(0, vrsImage);
context->Dispatch(vrsWidth, vrsHeight, 1);

// 3. レンダーパスでVRSイメージ使用
RHIRenderPassDesc passDesc;
passDesc.shadingRate.image = vrsImage;
passDesc.shadingRate.combiner = ERHIVRSCombiner::Max;
context->BeginRenderPass(passDesc);

// 描画...

context->EndRenderPass();
```

**アダプティブVRS (動的シーン):**
```cpp
// モーションベクトルに基づいVRSレートを決定
// 動きの速い領域は低品質、静止領域は高品質
void GenerateAdaptiveVRSImage(
    IRHICommandContext* context,
    IRHITexture* motionVectors,
    IRHITexture* vrsImage)
{
    context->SetComputePipeline(adaptiveVRSPipeline);
    context->SetSRV(0, motionVectors);
    context->SetUAV(0, vrsImage);
    context->Dispatch(...);
}
```

### VRSイメージフォーマット詳細

```
パレットベース (D3D12):
  各ピクセルは uint8 で、以下の値を持つ:
  0x00 = 1x1
  0x01 = 1x2
  0x04 = 2x1
  0x05 = 2x2
  0x06 = 2x4
  0x09 = 4x2
  0x0A = 4x4

フラクショナルベース (一部プラットフォーム):
  R8_UNORM または R16_FLOAT
  1.0 = 1x1 (フルレーチ
  0.5 = 2x2
  0.25 = 4x4
```

## 検証方法

- シェーディングレート設定の正確性
- VRSイメージサイズ計算の検証
- コンバイナの演算の検証
- 境界での品質遷移
- パフォーマンス向上の測定

## 備考

VRSは画質とパフォーマンスのトレードオフ、
適切を使用すればフレームレートを大幅に向上できるが、
不適切を使用は視覚的なアーティファクトを引き起こす、

VR/ARアプリケーションではフォビエーションと組み合わせて効果的、
