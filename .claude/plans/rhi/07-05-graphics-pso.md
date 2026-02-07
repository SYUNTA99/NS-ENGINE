# 07-05: グラフィックスPSO

## 目的

グラフィックスパイプラインステートオブジェクト（PSO）を定義する。

## 参照ドキュメント

- 07-01-blend-state.md (RHIBlendStateDesc)
- 07-02-rasterizer-state.md (RHIRasterizerStateDesc)
- 07-03-depth-stencil-state.md (RHIDepthStencilStateDesc)
- 07-04-input-layout.md (RHIInputLayoutDesc)
- 06-02-shader-interface.md (RHIGraphicsShaders)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIPipelineState.h`

## TODO

### 1. プリミティブトポロジー

```cpp
namespace NS::RHI
{
    /// プリミティブトポロジー
    enum class ERHIPrimitiveTopology : uint8
    {
        Undefined,
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,

        // テッセレーション用
        PatchList_1ControlPoint,
        PatchList_2ControlPoints,
        PatchList_3ControlPoints,
        PatchList_4ControlPoints,
        PatchList_5ControlPoints,
        PatchList_6ControlPoints,
        // ... 最大32コントロールポイント
        PatchList_32ControlPoints,

        // 隣接情報付き
        LineListAdj,
        LineStripAdj,
        TriangleListAdj,
        TriangleStripAdj,
    };

    /// プリミティブトポロジータイプ（パイプライン用）。
    enum class ERHIPrimitiveTopologyType : uint8
    {
        Undefined,
        Point,
        Line,
        Triangle,
        Patch,
    };

    /// トポロジーからタイプを取得
    inline ERHIPrimitiveTopologyType GetTopologyType(ERHIPrimitiveTopology topology)
    {
        switch (topology) {
            case ERHIPrimitiveTopology::PointList:
                return ERHIPrimitiveTopologyType::Point;
            case ERHIPrimitiveTopology::LineList:
            case ERHIPrimitiveTopology::LineStrip:
            case ERHIPrimitiveTopology::LineListAdj:
            case ERHIPrimitiveTopology::LineStripAdj:
                return ERHIPrimitiveTopologyType::Line;
            case ERHIPrimitiveTopology::TriangleList:
            case ERHIPrimitiveTopology::TriangleStrip:
            case ERHIPrimitiveTopology::TriangleListAdj:
            case ERHIPrimitiveTopology::TriangleStripAdj:
                return ERHIPrimitiveTopologyType::Triangle;
            default:
                if (topology >= ERHIPrimitiveTopology::PatchList_1ControlPoint &&
                    topology <= ERHIPrimitiveTopology::PatchList_32ControlPoints) {
                    return ERHIPrimitiveTopologyType::Patch;
                }
                return ERHIPrimitiveTopologyType::Undefined;
        }
    }
}
```

- [ ] ERHIPrimitiveTopology 列挙型
- [ ] ERHIPrimitiveTopologyType 列挙型
- [ ] GetTopologyType

### 2. レンダーターゲットフォーマット記述

```cpp
namespace NS::RHI
{
    /// レンダーターゲットフォーマット記述
    struct RHI_API RHIRenderTargetFormats
    {
        /// RTフォーマット
        EPixelFormat formats[kMaxRenderTargets] = {};

        /// 有効なRT数
        uint32 count = 0;

        /// デプスステンシルフォーマット
        EPixelFormat depthStencilFormat = EPixelFormat::Unknown;

        /// サンプル数
        ERHISampleCount sampleCount = ERHISampleCount::Count1;

        /// サンプル品質
        uint32 sampleQuality = 0;

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIRenderTargetFormats& SetRT(uint32 index, EPixelFormat format) {
            if (index < kMaxRenderTargets) {
                formats[index] = format;
                if (index >= count) count = index + 1;
            }
            return *this;
        }

        RHIRenderTargetFormats& SetDepthStencil(EPixelFormat format) {
            depthStencilFormat = format;
            return *this;
        }

        RHIRenderTargetFormats& SetSampleCount(ERHISampleCount sc, uint32 quality = 0) {
            sampleCount = sc;
            sampleQuality = quality;
            return *this;
        }

        //=====================================================================
        // プリセット
        //=====================================================================

        /// 単一RT + Depth
        static RHIRenderTargetFormats SingleRTWithDepth(
            EPixelFormat rtFormat,
            EPixelFormat dsFormat = EPixelFormat::D32_Float)
        {
            RHIRenderTargetFormats formats;
            formats.SetRT(0, rtFormat);
            formats.SetDepthStencil(dsFormat);
            return formats;
        }

        /// デプスのみ
        static RHIRenderTargetFormats DepthOnly(EPixelFormat dsFormat = EPixelFormat::D32_Float) {
            RHIRenderTargetFormats formats;
            formats.SetDepthStencil(dsFormat);
            return formats;
        }
    };
}
```

- [ ] RHIRenderTargetFormats 構造体

### 3. グラフィックスPSO記述

```cpp
namespace NS::RHI
{
    /// グラフィックスパイプラインステート記述
    struct RHI_API RHIGraphicsPipelineStateDesc
    {
        //=====================================================================
        // シェーダー
        //=====================================================================

        /// 頂点シェーダー（必要）
        IRHIShader* vertexShader = nullptr;

        /// ピクセルシェーダー
        IRHIShader* pixelShader = nullptr;

        /// ジオメトリシェーダー（オプション）。
        IRHIShader* geometryShader = nullptr;

        /// ハルシェーダー（テッセレーション用）。
        IRHIShader* hullShader = nullptr;

        /// ドメインシェーダー（テッセレーション用）。
        IRHIShader* domainShader = nullptr;

        //=====================================================================
        // ルートシグネチャ
        //=====================================================================

        /// ルートシグネチャ
        IRHIRootSignature* rootSignature = nullptr;

        //=====================================================================
        // 入力アセンブラー
        //=====================================================================

        /// 入力レイアウト
        RHIInputLayoutDesc inputLayout;

        /// プリミティブトポロジータイプ
        ERHIPrimitiveTopologyType primitiveTopologyType = ERHIPrimitiveTopologyType::Triangle;

        /// インデックスストリップカット値
        enum class IndexBufferStripCutValue : uint8 {
            Disabled,
            MaxUInt16,
            MaxUInt32,
        };
        IndexBufferStripCutValue stripCutValue = IndexBufferStripCutValue::Disabled;

        //=====================================================================
        // レンダーステート
        //=====================================================================

        /// ラスタライザーステート
        RHIRasterizerStateDesc rasterizerState;

        /// ブレンドステート
        RHIBlendStateDesc blendState;

        /// デプスステンシルステート
        RHIDepthStencilStateDesc depthStencilState;

        /// サンプルマスク
        uint32 sampleMask = 0xFFFFFFFF;

        //=====================================================================
        // 出力
        //=====================================================================

        /// レンダーターゲットフォーマット
        RHIRenderTargetFormats renderTargetFormats;

        //=====================================================================
        // その他
        //=====================================================================

        /// ノードマスク（マルチGPU用）。
        uint32 nodeMask = 0;

        /// フラグ
        enum class Flags : uint32 {
            None = 0,
            /// ツール/デバッグ用
            ToolDebug = 1 << 0,
        };
        Flags flags = Flags::None;

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIGraphicsPipelineStateDesc& SetVS(IRHIShader* vs) { vertexShader = vs; return *this; }
        RHIGraphicsPipelineStateDesc& SetPS(IRHIShader* ps) { pixelShader = ps; return *this; }
        RHIGraphicsPipelineStateDesc& SetGS(IRHIShader* gs) { geometryShader = gs; return *this; }
        RHIGraphicsPipelineStateDesc& SetHS(IRHIShader* hs) { hullShader = hs; return *this; }
        RHIGraphicsPipelineStateDesc& SetDS(IRHIShader* ds) { domainShader = ds; return *this; }
        RHIGraphicsPipelineStateDesc& SetRootSignature(IRHIRootSignature* rs) { rootSignature = rs; return *this; }
        RHIGraphicsPipelineStateDesc& SetInputLayout(const RHIInputLayoutDesc& il) { inputLayout = il; return *this; }
        RHIGraphicsPipelineStateDesc& SetTopologyType(ERHIPrimitiveTopologyType type) { primitiveTopologyType = type; return *this; }
        RHIGraphicsPipelineStateDesc& SetRasterizerState(const RHIRasterizerStateDesc& rs) { rasterizerState = rs; return *this; }
        RHIGraphicsPipelineStateDesc& SetBlendState(const RHIBlendStateDesc& bs) { blendState = bs; return *this; }
        RHIGraphicsPipelineStateDesc& SetDepthStencilState(const RHIDepthStencilStateDesc& dss) { depthStencilState = dss; return *this; }
        RHIGraphicsPipelineStateDesc& SetRenderTargetFormats(const RHIRenderTargetFormats& rtf) { renderTargetFormats = rtf; return *this; }
    };
    RHI_ENUM_CLASS_FLAGS(RHIGraphicsPipelineStateDesc::Flags)
}
```

- [ ] RHIGraphicsPipelineStateDesc 構造体
- [ ] ビルダーメソッド

### 4. IRHIGraphicsPipelineState インターフェース

```cpp
namespace NS::RHI
{
    /// グラフィックスパイプラインステート
    class RHI_API IRHIGraphicsPipelineState : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(GraphicsPipelineState)

        virtual ~IRHIGraphicsPipelineState() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// ルートシグネチャ取得
        virtual IRHIRootSignature* GetRootSignature() const = 0;

        /// プリミティブトポロジータイプ取得
        virtual ERHIPrimitiveTopologyType GetPrimitiveTopologyType() const = 0;

        //=====================================================================
        // シェーダー情報
        //=====================================================================

        /// 頂点シェーダー取得
        virtual IRHIShader* GetVertexShader() const = 0;

        /// ピクセルシェーダー取得
        virtual IRHIShader* GetPixelShader() const = 0;

        /// テッセレーション使用か
        bool UsesTessellation() const {
            return GetPrimitiveTopologyType() == ERHIPrimitiveTopologyType::Patch;
        }

        //=====================================================================
        // キャッシュ
        //=====================================================================

        /// キャッシュ済みバイナリ取得（プラットフォーム依存）
        virtual RHIShaderBytecode GetCachedBlob() const = 0;
    };

    using RHIGraphicsPipelineStateRef = TRefCountPtr<IRHIGraphicsPipelineState>;
}
```

- [ ] IRHIGraphicsPipelineState インターフェース

### 5. PSO作成・キャッシュ

```cpp
namespace NS::RHI
{
    /// PSO作成（RHIDeviceに追加）。
    ///
    /// 作成時検証（デバッグビルドのみ）:
    /// 1. 各シェーダーからリフレクションを取得
    /// 2. RootSignatureのパラメータがシェーダーの要求するバインディングを全て含むか検証
    /// 3. ShaderVisibilityフラグが実際のシェーダーステージと一致するか検証
    /// 4. InputLayoutのセマンティクスがVS入力シグネチャと一致するか検証
    /// 5. VS出力シグネチャとPS入力シグネチャのセマンティクス/型が一致するか検証
    ///    （HS/DS/GSが存在する場合は各ステージ間も同様）
    /// 検証失敗時はRHI_ASSERTでフェイルファスト
    class IRHIDevice
    {
    public:
        /// グラフィックスPSO作成
        virtual IRHIGraphicsPipelineState* CreateGraphicsPipelineState(
            const RHIGraphicsPipelineStateDesc& desc,
            const char* debugName = nullptr) = 0;

        /// キャッシュ済みBlobからPSO作成
        /// @note cachedBlobのシェーダーハッシュがdescのシェーダーバイトコードと一致しない場合、
        ///       キャッシュを無視して再作成する
        virtual IRHIGraphicsPipelineState* CreateGraphicsPipelineStateFromCache(
            const RHIGraphicsPipelineStateDesc& desc,
            const RHIShaderBytecode& cachedBlob,
            const char* debugName = nullptr) = 0;
    };

    /// PSOキャッシュ
    /// キー生成規則: シェーダーバイトコードハッシュ + ステート値のハッシュを使用。
    /// シェーダーオブジェクトのポインタではなくバイトコード内容でキーを生成すること。
    /// 同一バイトコードを持つ異なるシェーダーオブジェクトは同一PSOに解決される。
    ///
    /// 3レベルキャッシュ:
    /// 1. HighLevel: RHIGraphicsPipelineStateDesc ハッシュ → PSO
    /// 2. LowLevel: ネイティブPSO blob → PSO
    /// 3. Disk: ファイルシステム上のPSOバイナリ（SaveToFile/LoadFromFile）
    ///
    /// スレッド安全性:
    /// - FindPipelineState(): 複数スレッドから同時呼び出し可能（読み取りロック）
    /// - AddPipelineState(): 単一スレッドまたは排他ロック下で呼び出すこと
    /// - 実装にはstd::shared_mutexを使用し、読み取りは共有ロック、
    ///   書き込みは排他ロックとする
    /// - 非同期PSO作成時: 作成完了後にAddPipelineState()を排他ロック下で呼び出す
    class RHI_API IRHIPipelineStateCache
    {
    public:
        virtual ~IRHIPipelineStateCache() = default;

        /// キャッシュに追加
        virtual void AddPipelineState(
            const void* descHash,
            size_t hashSize,
            IRHIGraphicsPipelineState* pso) = 0;

        /// キャッシュから検索
        virtual IRHIGraphicsPipelineState* FindPipelineState(
            const void* descHash,
            size_t hashSize) = 0;

        /// ファイルに保存
        virtual bool SaveToFile(const char* path) = 0;

        /// ファイルから読み込み
        virtual bool LoadFromFile(const char* path) = 0;

        /// キャッシュクリア
        virtual void Clear() = 0;

        /// エントリ数取得
        virtual uint32 GetEntryCount() const = 0;
    };
}
```

- [ ] CreateGraphicsPipelineState
- [ ] CreateGraphicsPipelineStateFromCache
- [ ] IRHIPipelineStateCache

## 検証方法

- [ ] PSO作成の正常動作
- [ ] 各ステートの適用確認
- [ ] キャッシュの動作確認
- [ ] シェーダーとの整合性
- [ ] RootSignature互換性検証（シェーダーリフレクション照合）
- [ ] PSOキャッシュのバイトコードベースハッシュ重複排除
- [ ] PSOキャッシュの並行読み取り安全性
