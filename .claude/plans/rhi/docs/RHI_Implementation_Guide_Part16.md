# RHI Implementation Guide - Part 16: 列挙型と型リファレンス

## 概要

このドキュメントでは、RHIシステムで使用されるすべての列挙型、定数、および基本的な型定義の完全なリファレンスを提供します。

---

## 1. RHIインターフェースと機能サポート

### 1.1 ERHIInterfaceType

**場所**: `RHIDefinitions.h:156`

RHIバックエンドの種類を定義。

```cpp
enum class ERHIInterfaceType
{
    Hidden,     // 非表示/内部使用
    Null,       // Nullレンダラー（ヘッドレスモード用）
    D3D11,      // Direct3D 11
    D3D12,      // Direct3D 12
    Vulkan,     // Vulkan
    Metal,      // Apple Metal
    Agx,        // Apple AGX（Appleシリコン専用Metal）
    OpenGL,     // OpenGL/OpenGL ES
};
```

### 1.2 ERHIFeatureSupport

**場所**: `RHIDefinitions.h:168`

機能のサポートレベルを定義。

```cpp
enum class ERHIFeatureSupport : uint8
{
    // 実行時に利用不可
    Unsupported,

    // ハードウェア/ドライバーに依存
    RuntimeDependent,

    // 実行時に必ず利用可能
    RuntimeGuaranteed,

    Num,
    NumBits = 2,
};
```

### 1.3 ERHIStaticShaderBindingLayoutSupport

**場所**: `RHIDefinitions.h:192`

静的シェーダーバインディングレイアウトのサポートレベル。

```cpp
enum class ERHIStaticShaderBindingLayoutSupport : uint8
{
    Unsupported,       // 非サポート
    RayTracingOnly,    // RTシェーダーのみ
    AllShaderTypes,    // 全シェーダータイプ

    NumBits = 2
};
```

---

## 2. シェーダー関連

### 2.1 EShaderFrequency

**場所**: `RHIDefinitions.h:201`

シェーダーステージの種類。

```cpp
enum EShaderFrequency : uint8
{
    SF_Vertex               = 0,   // 頂点シェーダー
    SF_Mesh                 = 1,   // メッシュシェーダー
    SF_Amplification        = 2,   // アンプリフィケーションシェーダー
    SF_Pixel                = 3,   // ピクセル/フラグメントシェーダー
    SF_Geometry             = 4,   // ジオメトリシェーダー
    SF_Compute              = 5,   // コンピュートシェーダー
    SF_RayGen               = 6,   // レイ生成シェーダー
    SF_RayMiss              = 7,   // レイミスシェーダー
    SF_RayHitGroup          = 8,   // レイヒットグループ
    SF_RayCallable          = 9,   // レイコーラブルシェーダー
    SF_WorkGraphRoot        = 10,  // ワークグラフルートシェーダー
    SF_WorkGraphComputeNode = 11,  // ワークグラフコンピュートノード

    SF_NumFrequencies       = 12,  // 総数

    // グラフィックスパイプライン用（コンピュート除く）
    SF_NumGraphicsFrequencies = 5,

    // 標準シェーダー数（コンピュート含む）
    SF_NumStandardFrequencies = 6,

    SF_NumBits = 4,
};
```

### 2.2 ERHIShaderBundleMode

**場所**: `RHIResources.h:3895`

シェーダーバンドルのモード。

```cpp
enum class ERHIShaderBundleMode : uint8
{
    CS,     // コンピュートシェーダー（Load3、ArgStride >= 12）
    MSPS,   // メッシュ+ピクセルシェーダー（Load、ArgStride >= 4）
    VSPS,   // 頂点+ピクセルシェーダー（Load4、ArgStride >= 16）
    MAX
};
```

---

## 3. デスクリプタシステム

### 3.1 ERHIDescriptorHeapType

**場所**: `RHIDefinitions.h:1337`

デスクリプタヒープの種類。

```cpp
enum class ERHIDescriptorHeapType : uint8
{
    Standard,       // SRV/UAV/CBV用
    Sampler,        // サンプラー用
    RenderTarget,   // RTV用
    DepthStencil,   // DSV用
    Count,
    Invalid = MAX_uint8
};
```

### 3.2 ERHIDescriptorType

**場所**: `RHIDefinitions.h:1347`

デスクリプタの種類。

```cpp
enum class ERHIDescriptorType : uint8
{
    BufferSRV,              // バッファSRV
    BufferUAV,              // バッファUAV
    TypedBufferSRV,         // 型付きバッファSRV
    TypedBufferUAV,         // 型付きバッファUAV
    TextureSRV,             // テクスチャSRV
    TextureUAV,             // テクスチャUAV
    CBV,                    // 定数バッファビュー
    Sampler,                // サンプラー
    AccelerationStructure,  // レイトレーシングAS
    Invalid
};
```

### 3.3 ERHIDescriptorTypeMask

**場所**: `RHIDefinitions.h:1366`

デスクリプタタイプのビットマスク。

```cpp
enum class ERHIDescriptorTypeMask : uint16
{
    None = 0,

    BufferSRV      = (1 << 0),
    BufferUAV      = (1 << 1),
    TypedBufferSRV = (1 << 2),
    TypedBufferUAV = (1 << 3),
    TextureSRV     = (1 << 4),
    TextureUAV     = (1 << 5),
    CBV            = (1 << 6),
    Sampler        = (1 << 7),
    AccelerationStructure = (1 << 8),

    // 複合マスク
    SRV = (BufferSRV | TypedBufferSRV | TextureSRV),
    UAV = (BufferUAV | TypedBufferUAV | TextureUAV),
    Standard = (SRV | UAV | CBV | AccelerationStructure),
};
ENUM_CLASS_FLAGS(ERHIDescriptorTypeMask);
```

### 3.4 FRHIDescriptorHandle

**場所**: `RHIDefinitions.h:1400`

デスクリプタハンドル構造体。

```cpp
struct FRHIDescriptorHandle
{
    FRHIDescriptorHandle() = default;
    FRHIDescriptorHandle(ERHIDescriptorType InType, uint32 InIndex);

    uint32 GetIndex() const;
    ERHIDescriptorType GetType() const;
    bool IsValid() const;

private:
    uint32             Index{ MAX_uint32 };
    ERHIDescriptorType Type{ ERHIDescriptorType::Invalid };
};
```

---

## 4. バインドレス設定

### 4.1 ERHIBindlessConfiguration

**場所**: `RHIDefinitions.h:1431`

バインドレスレンダリングの設定レベル。

```cpp
enum class ERHIBindlessConfiguration
{
    // バインドレス無効
    Disabled,

    // RTシェーダーのみ
    RayTracing,

    // RT + マテリアル + オプトインシェーダー
    Minimal,

    // 全シェーダー
    All
};
```

**ヘルパー関数**:
```cpp
bool IsBindlessDisabled(ERHIBindlessConfiguration Configuration);
bool IsBindlessEnabledForOnlyRayTracing(ERHIBindlessConfiguration Configuration);
bool IsBindlessEnabledForRayTracing(ERHIBindlessConfiguration Configuration);
bool IsBindlessEnabledForAnyGraphics(ERHIBindlessConfiguration Configuration);
bool IsBindlessFullyEnabled(ERHIBindlessConfiguration Configuration);
```

---

## 5. リソース遷移

### 5.1 ERHITransitionCreateFlags

**場所**: `RHIDefinitions.h:1494`

リソース遷移作成フラグ。

```cpp
enum class ERHITransitionCreateFlags
{
    None = 0,

    // パイプライン間フェンスを無効化
    NoFence = 1 << 0,

    // Begin/End間に有効な作業がない場合、部分フラッシュを使用
    NoSplit = 1 << 1,

    // RenderPass中の遷移を許可
    AllowDuringRenderPass = 1 << 2,

    // ERHIPipeline::Allからの単一パイプライン遷移を許可（高度な使用）
    AllowDecayPipelines = 1 << 3
};
ENUM_CLASS_FLAGS(ERHITransitionCreateFlags);
```

### 5.2 EResourceTransitionFlags

**場所**: `RHIDefinitions.h:1516`

リソース遷移の追加フラグ。

```cpp
enum class EResourceTransitionFlags : uint8
{
    None = 0,

    // 圧縮状態を維持（解凍しない）
    MaintainCompression = 1 << 0,

    // After状態をトラッカーで無視
    IgnoreAfterState = 1 << 1,

    Last = IgnoreAfterState,
    Mask = (Last << 1) - 1
};
ENUM_CLASS_FLAGS(EResourceTransitionFlags);
```

### 5.3 ERHIAccess

**場所**: `RHIAccess.h:10`

リソースアクセス状態（32ビットフラグ）。

```cpp
enum class ERHIAccess : uint32
{
    Unknown            = 0,

    // 読み取りアクセス
    CPURead            = 1 << 0,
    Present            = 1 << 1,
    IndirectArgs       = 1 << 2,
    VertexOrIndexBuffer = 1 << 3,
    SRVCompute         = 1 << 4,
    SRVGraphics        = 1 << 5,
    CopySrc            = 1 << 6,
    ResolveSrc         = 1 << 7,
    DSVRead            = 1 << 8,
    ShadingRateSource  = 1 << 9,
    BVHRead            = 1 << 10,

    // 書き込みアクセス
    UAVCompute         = 1 << 11,
    UAVGraphics        = 1 << 12,
    RTV                = 1 << 13,
    CopyDest           = 1 << 14,
    ResolveDst         = 1 << 15,
    DSVWrite           = 1 << 16,
    BVHWrite           = 1 << 17,

    // 複合
    SRVMask = SRVCompute | SRVGraphics,
    UAVMask = UAVCompute | UAVGraphics,
    ReadOnlyMask = CPURead | Present | IndirectArgs | VertexOrIndexBuffer |
                   SRVMask | CopySrc | ResolveSrc | DSVRead |
                   ShadingRateSource | BVHRead,
    WritableMask = UAVMask | RTV | CopyDest | ResolveDst | DSVWrite | BVHWrite,

    // すべて
    Mask = ReadOnlyMask | WritableMask,
};
ENUM_CLASS_FLAGS(ERHIAccess);
```

---

## 6. パイプラインとコンテキスト

### 6.1 ERHIPipeline

**場所**: `RHIPipeline.h:12`

パイプラインの種類。

```cpp
enum class ERHIPipeline : uint8
{
    Graphics     = 1 << 0,  // グラフィックスパイプライン
    AsyncCompute = 1 << 1,  // 非同期コンピュート

    None = 0,
    All = Graphics | AsyncCompute,
    Num = 2,

    NumBits = 2
};
ENUM_CLASS_FLAGS(ERHIPipeline);
```

### 6.2 ERHIThreadMode

**場所**: `RHICommandList.h:99`

RHIスレッドモード。

```cpp
enum class ERHIThreadMode
{
    // 単一スレッドモード
    SingleThreaded,

    // RHIスレッド使用（別スレッドで実行）
    DedicatedThread,

    // タスクベース（タスクシステムで実行）
    TaskBased
};
```

### 6.3 ERHISubmitFlags

**場所**: `RHICommandList.h:4593`

コマンドリスト送信フラグ。

```cpp
enum class ERHISubmitFlags
{
    None = 0,

    // バッチの最後のコマンドリスト
    EndOfBatch = 1 << 0,

    // GPU作業の最後
    EndOfFrame = 1 << 1,

    // フェンスシグナルを強制
    ForceFenceSignal = 1 << 2
};
ENUM_CLASS_FLAGS(ERHISubmitFlags);
```

---

## 7. サンプラーと描画状態

### 7.1 ESamplerFilter

**場所**: `RHIDefinitions.h:325`

サンプラーフィルターモード。

```cpp
enum ESamplerFilter
{
    SF_Point,           // ポイントフィルタリング
    SF_Bilinear,        // バイリニアフィルタリング
    SF_Trilinear,       // トライリニアフィルタリング
    SF_AnisotropicPoint,  // 異方性ポイント
    SF_AnisotropicLinear, // 異方性リニア

    ESamplerFilter_Num,
    ESamplerFilter_NumBits = 3,
};
```

### 7.2 ESamplerAddressMode

**場所**: `RHIDefinitions.h:338`

サンプラーアドレスモード。

```cpp
enum ESamplerAddressMode
{
    AM_Wrap,    // ラップ（繰り返し）
    AM_Clamp,   // クランプ（端で固定）
    AM_Mirror,  // ミラー（反転繰り返し）
    AM_Border,  // ボーダー（一部プラットフォームのみ）

    ESamplerAddressMode_Num,
    ESamplerAddressMode_NumBits = 2,
};
```

### 7.3 ERHIZBuffer

**場所**: `RHIDefinitions.h:298`

Zバッファ設定。

```cpp
enum class ERHIZBuffer
{
    FarPlane = 0,   // 遠クリップ面の値
    NearPlane = 1,  // 近クリップ面の値

    // 反転Zバッファかどうか（UEは反転Z使用）
    IsInverted = (int32)((int32)FarPlane < (int32)NearPlane),
};
```

---

## 8. リソース初期化

### 8.1 ERHIBufferInitAction

**場所**: `RHIResources.h:1401`

バッファ初期化アクション。

```cpp
enum class ERHIBufferInitAction
{
    // 初期化なし（内容は未定義）
    NoAction,

    // データでの初期化
    Create,

    // コピーによる初期化
    CreateAndCopyData
};
```

### 8.2 ERHITextureInitAction

**場所**: `RHIResources.h:1925`

テクスチャ初期化アクション。

```cpp
enum class ERHITextureInitAction
{
    // 初期化なし
    NoAction,

    // クリア値で初期化
    Clear,

    // データで初期化
    Create
};
```

### 8.3 ERHITexturePlane

**場所**: `RHIResources.h:2573`

テクスチャプレーン指定。

```cpp
enum class ERHITexturePlane : uint8
{
    Primary,    // メインカラープレーン
    Stencil,    // ステンシルプレーン（深度ステンシルの場合）
    HTile,      // AMD HTileメタデータ
    CMask,      // AMD CMaskメタデータ
    FMask,      // AMD FMaskメタデータ
};
```

---

## 9. Transient リソース

### 9.1 ERHITransientResourceType

**場所**: `RHITransientResourceAllocator.h:177`

一時リソースの種類。

```cpp
enum class ERHITransientResourceType : uint8
{
    Texture,  // テクスチャ
    Buffer,   // バッファ
};
```

### 9.2 ERHITransientAllocationType

**場所**: `RHITransientResourceAllocator.h:183`

一時リソースの割り当て方式。

```cpp
enum class ERHITransientAllocationType : uint8
{
    Heap,   // ヒープ割り当て
    Page,   // ページ割り当て
};
```

### 9.3 ERHITransientResourceCreateMode

**場所**: `RHITransientResourceAllocator.h:526`

一時リソース作成モード。

```cpp
enum class ERHITransientResourceCreateMode
{
    Acquire,  // 取得（エイリアシング開始）
    Discard,  // 破棄（エイリアシング終了）
};
```

---

## 10. 同期とバジェット

### 10.1 ESyncComputeBudget

**場所**: `RHIDefinitions.h:1330`

同期コンピュートのバジェット設定。

```cpp
enum class ESyncComputeBudget
{
    // デフォルト（RHI選択）
    Default,

    // バランス（グラフィックス/コンピュート並行実行）
    Balanced,

    // コンピュート重視（同期コンピュートにリソース優先）
    ComputeHeavy,
};
```

---

## 11. カラースペースとHDR

### 11.1 EColorSpaceAndEOTF

**場所**: `RHIDefinitions.h:1470`

カラースペースと伝達関数の組み合わせ。

```cpp
enum class EColorSpaceAndEOTF
{
    EUnknown = 0,

    // カラースペース（下位4ビット）
    EColorSpace_Rec709  = 1,    // Rec.709（sRGB）
    EColorSpace_Rec2020 = 2,    // Rec.2020（HDR/WCG）
    EColorSpace_DCIP3   = 3,    // DCI-P3（シネマ）
    EEColorSpace_MASK   = 0xf,

    // 伝達関数（上位4ビット）
    EEOTF_Linear = 1 << 4,  // リニア
    EEOTF_sRGB   = 2 << 4,  // sRGBガンマ
    EEOTF_PQ     = 3 << 4,  // PQ（HDR10）
    EEOTF_MASK   = 0xf << 4,

    // 一般的な組み合わせ
    ERec709_sRGB    = EColorSpace_Rec709  | EEOTF_sRGB,    // 標準SDR
    ERec709_Linear  = EColorSpace_Rec709  | EEOTF_Linear,  // リニアsRGB
    ERec2020_PQ     = EColorSpace_Rec2020 | EEOTF_PQ,      // HDR10
    ERec2020_Linear = EColorSpace_Rec2020 | EEOTF_Linear,  // リニアHDR
    EDCIP3_PQ       = EColorSpace_DCIP3   | EEOTF_PQ,      // DCI-P3 HDR
    EDCIP3_Linear   = EColorSpace_DCIP3   | EEOTF_Linear,  // リニアP3
};
```

---

## 12. クエリとプロファイリング

### 12.1 ERenderQueryType

**場所**: `RHIDefinitions.h:257`

レンダークエリの種類。

```cpp
enum ERenderQueryType
{
    // 未定義（フレームイベント待機等）
    RQT_Undefined,

    // オクルージョンクエリ（カリングされていないサンプル数）
    RQT_Occlusion,

    // 絶対時間（マイクロ秒）
    RQT_AbsoluteTime,
};
```

---

## 13. シェーディングパス

### 13.1 ERHIShadingPath

**場所**: `RHIDefinitions.h:314`

シェーディングパスの種類。

```cpp
namespace ERHIShadingPath
{
    enum Type : int
    {
        Deferred,  // ディファードレンダリング
        Forward,   // フォワードレンダリング
        Mobile,    // モバイル最適化
        Num
    };
}
```

---

## 14. 重要な定数

### 14.1 リソース制限

```cpp
// テクスチャの最大MIPレベル数
enum { MAX_TEXTURE_MIP_COUNT = 15 };

// メッシュの最大LOD数
enum { MAX_MESH_LOD_COUNT = 8 };

// 頂点宣言の最大要素数
enum { MaxVertexElementCount = 17 };

// シェーダー配列要素のアライメント
enum { ShaderArrayElementAlignBytes = 16 };

// 同時レンダーターゲット数
enum { MaxSimultaneousRenderTargets = 8 };

// イミュータブルサンプラーの最大数
enum { MaxImmutableSamplers = 2 };
```

### 14.2 GPU関連

```cpp
// 最大GPU数（マルチGPU用）
enum { MAX_NUM_GPUS = 4 };

// デフォルトのReservedResourceタイルサイズ
constexpr uint32 DefaultReservedResourceTileSize = 65536; // 64KB
```

---

## 15. マスク操作ヘルパー

### 15.1 FRHIGPUMask

**場所**: `MultiGPU.h:32`

マルチGPU環境でのGPUマスク。

```cpp
struct FRHIGPUMask
{
    // 単一GPU
    static FRHIGPUMask GPU0();
    static FRHIGPUMask GPU1();

    // 全GPU
    static FRHIGPUMask All();

    // ビット操作
    bool HasSingleIndex() const;
    uint32 ToIndex() const;
    bool Contains(uint32 Index) const;

    // イテレーション
    FIterator begin() const;
    FIterator end() const;
};
```

### 15.2 FRHIScopedGPUMask

**場所**: `RHICommandList.h:5093`

スコープ付きGPUマスク設定。

```cpp
struct FRHIScopedGPUMask
{
    FRHIScopedGPUMask(FRHICommandListBase& RHICmdList, FRHIGPUMask InGPUMask);
    ~FRHIScopedGPUMask();

private:
    FRHICommandListBase& RHICmdList;
    FRHIGPUMask PreviousMask;
};
```

---

## まとめ

Part 16では、RHIシステムで使用される主要な列挙型と型定義を網羅しました：

| カテゴリ | 列挙型/型 |
|----------|-----------|
| インターフェース | ERHIInterfaceType, ERHIFeatureSupport |
| シェーダー | EShaderFrequency, ERHIShaderBundleMode |
| デスクリプタ | ERHIDescriptorHeapType, ERHIDescriptorType, FRHIDescriptorHandle |
| バインドレス | ERHIBindlessConfiguration |
| リソース遷移 | ERHITransitionCreateFlags, EResourceTransitionFlags, ERHIAccess |
| パイプライン | ERHIPipeline, ERHIThreadMode, ERHISubmitFlags |
| サンプラー | ESamplerFilter, ESamplerAddressMode |
| 初期化 | ERHIBufferInitAction, ERHITextureInitAction |
| Transient | ERHITransientResourceType, ERHITransientAllocationType |
| カラー | EColorSpaceAndEOTF |
| マルチGPU | FRHIGPUMask |

これらの型を適切に使用することで、クロスプラットフォームで動作する堅牢なRHIコードを記述できます。
