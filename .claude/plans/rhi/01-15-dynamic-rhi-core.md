# 01-15: IDynamicRHI基本インターフェース

## 目的

RHI最上位オブジェクトの基本インターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (2. FDynamicRHI実装詳細)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IDynamicRHI.h`

## TODO

### 1. IDynamicRHI: 基本定義

```cpp
#pragma once

#include "RHIFwd.h"
#include "RHIEnums.h"
#include "RHITypes.h"

namespace NS::RHI
{
    /// RHI最上位インターフェース
    /// プラットフォーム固有RHI（D3D12, Vulkan等）の基底
    class RHI_API IDynamicRHI
    {
    public:
        virtual ~IDynamicRHI() = default;

        //=====================================================================
        // 識別
        //=====================================================================

        /// RHIバックエンド種別取得
        virtual ERHIInterfaceType GetInterfaceType() const = 0;

        /// RHI名取得（D3D12", "Vulkan"等）
        virtual const char* GetName() const = 0;

        /// 現在の機能レベル取得
        virtual ERHIFeatureLevel GetFeatureLevel() const = 0;
    };
}
```

- [ ] 基本インターフェース定義
- [ ] 識別メソッド

### 2. IDynamicRHI: ライフサイクル

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // ライフサイクル
        //=====================================================================

        /// 初期化
        /// アダプター/デバイス/キューを作成
        /// @return 成功時true
        virtual bool Init() = 0;

        /// 追加初期化（Init後、レンダリング開始前）。
        /// ピクセルフォーマット情報等の設定
        virtual void PostInit() {}

        /// シャットダウン
        /// 全リソース解放、デバイス破棄
        virtual void Shutdown() = 0;

        /// 毎フレーム更新
        /// メモリ統計、タイムアウト処理等。
        virtual void Tick(float deltaTime) { (void)deltaTime; }

        /// フレーム終了処理
        /// リソース解放、統計収集
        virtual void EndFrame() {}

        /// RHIが有効か
        virtual bool IsInitialized() const = 0;
    };
}
```

- [ ] Init / PostInit / Shutdown
- [ ] Tick / EndFrame
- [ ] IsInitialized

### 3. IDynamicRHI: アダプター/デバイスアクセス

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // アダプター/デバイスアクセス
        //=====================================================================

        /// 利用可能なアダプター数
        virtual uint32 GetAdapterCount() const = 0;

        /// アダプター取得
        /// @param index アダプターインデックス
        /// @return アダプター（存在しない場合nullptr）。
        virtual IRHIAdapter* GetAdapter(uint32 index) const = 0;

        /// 現在使用中のアダプター取得
        virtual IRHIAdapter* GetCurrentAdapter() const = 0;

        /// デフォルトデバイス取得
        /// シングルGPU環境界主要デバイス
        virtual IRHIDevice* GetDefaultDevice() const = 0;

        /// GPUマスクからデバイス取得
        /// @param gpuMask 対象GPUマスク
        /// @return 最初にマッチするデバイス
        virtual IRHIDevice* GetDevice(GPUMask gpuMask) const = 0;
    };
}
```

- [ ] GetAdapterCount / GetAdapter
- [ ] GetCurrentAdapter
- [ ] GetDefaultDevice / GetDevice

### 4. IDynamicRHI: 機能クエリ

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // 機能クエリ
        //=====================================================================

        /// 機能サポート状態取得
        virtual ERHIFeatureSupport GetFeatureSupport(ERHIFeature feature) const = 0;

        /// 拡張サポート確認
        /// @param extensionName 拡張合
        virtual bool SupportsExtension(const char* extensionName) const = 0;

        //=====================================================================
        // 制限値取得
        //=====================================================================

        /// 最大テクスチャサイズ
        virtual uint32 GetMaxTextureSize() const = 0;

        /// 最大テクスチャ配列サイズ
        virtual uint32 GetMaxTextureArrayLayers() const = 0;

        /// 最大バッファサイズ
        virtual uint64 GetMaxBufferSize() const = 0;

        /// 最大コンスタントバッファサイズ
        virtual uint32 GetMaxConstantBufferSize() const = 0;

        /// 最大サンプルカウント
        virtual ERHISampleCount GetMaxSampleCount() const = 0;
    };

    /// 機能フラグ
    /// @note リファレンスドキュメント Part14 の機能一覧に基づく。
    ///       IDynamicRHI::Init() 内でアダプター機能クエリを実行し、各フラグをポピュレートする。
    ///       PostInit()での追加設定は不要。
    enum class ERHIFeature : uint32
    {
        // シェーダー機能
        WaveOperations,
        RayTracing,
        MeshShaders,
        VariableRateShading,
        AmplificationShaders,       ///< メッシュシェーダーパイプライン用
        ShaderModel6_6,             ///< SM 6.6+ 動的リソース
        ShaderModel6_7,             ///< SM 6.7+ (Work Graphs等)

        // テクスチャ機能
        TextureCompressionBC,
        TextureCompressionASTC,
        Texture3D,
        MSAA_16X,                   ///< 16X マルチサンプル
        SamplerFeedback,            ///< テクスチャストリーミング最適化

        // バッファ機能
        StructuredBuffer,
        ByteAddressBuffer,
        TypedBuffer,

        // レンダリング機能
        Bindless,
        ConservativeRasterization,
        MultiDrawIndirect,
        DrawIndirectCount,
        RenderPass,                 ///< D3D12 Render Pass対応
        DepthBoundsTest,            ///< デプスバウンドテスト

        // 高度な機能
        WorkGraphs,                 ///< D3D12 Work Graphs
        EnhancedBarriers,           ///< D3D12 Enhanced Barriers (Agility SDK 1.7+)
        GPUUploadHeaps,             ///< GPU Upload Heaps
        ExecuteIndirect,            ///< D3D12 ExecuteIndirect（拡張版）
        AtomicInt64,                ///< シェーダー内64bitアトミック
        Residency,                  ///< GPUレジデンシー管理（01-12 IsResident()と連携）

        Count
    };

    /// GetFeatureSupport() 実装パターン:
    /// 内部テーブル配列 `ERHIFeatureSupport m_featureTable[ERHIFeature::Count]` を使用。
    /// Init()内でアダプター機能クエリ結果を基にテーブルをポピュレートする。
}
```

- [ ] GetFeatureSupport
- [ ] 制限値取得メソッド
- [ ] ERHIFeature 列挙

### 5. IDynamicRHI: グローバルアクセス

```cpp
namespace NS::RHI
{
    //=========================================================================
    // グローバルRHIインスタンス
    //=========================================================================

    /// グローバルRHIポインタ
    extern RHI_API IDynamicRHI* g_dynamicRHI;

    /// グローバルRHI取得
    inline IDynamicRHI* GetDynamicRHI()
    {
        RHI_CHECK(g_dynamicRHI != nullptr);
        return g_dynamicRHI;
    }

    /// RHIが利用可能か
    inline bool IsRHIAvailable()
    {
        return g_dynamicRHI != nullptr && g_dynamicRHI->IsInitialized();
    }

    /// デフォルトデバイス取得ショートカット
    inline IRHIDevice* GetRHIDevice()
    {
        return GetDynamicRHI()->GetDefaultDevice();
    }
}
```

- [ ] g_dynamicRHI グローバル変数
- [ ] GetDynamicRHI アクセサ
- [ ] 便利なショートカット関数

## 検証方法

- [ ] インターフェース定義の完全性
- [ ] 派生クラスでの実装可能性
