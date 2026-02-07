# 01-16: IDynamicRHIリソースファクトリ

## 目的

IDynamicRHIのリソース作成メソッドを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (リソースファクトリ)
- docs/RHI/RHI_Implementation_Guide_Part2.md (リソース作成)

## 変更対象ファイル

更新:
- `Source/Engine/RHI/Public/IDynamicRHI.h`

## TODO

### 1. バッファ/テクスチャ作成

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // バッファ作成
        //=====================================================================

        /// バッファ作成
        /// @param desc バッファ記述情報
        /// @param initialData 初期データ（nullptrで初期化なし）
        /// @return 作成されたバッファ
        virtual TRefCountPtr<IRHIBuffer> CreateBuffer(
            const RHIBufferDesc& desc,
            const void* initialData = nullptr) = 0;

        //=====================================================================
        // テクスチャ作成
        //=====================================================================

        /// テクスチャ作成
        /// @param desc テクスチャ記述情報
        /// @return 作成されたテクスチャ
        virtual TRefCountPtr<IRHITexture> CreateTexture(
            const RHITextureDesc& desc) = 0;

        /// テクスチャ作成（初期データ付き）。
        /// @param desc テクスチャ記述情報
        /// @param initialData サブリソースごとの初期データ配列
        /// @param numSubresources サブリソース数
        virtual TRefCountPtr<IRHITexture> CreateTextureWithData(
            const RHITextureDesc& desc,
            const RHISubresourceData* initialData,
            uint32 numSubresources) = 0;
    };
}
```

- [ ] CreateBuffer
- [ ] CreateTexture / CreateTextureWithData

### 2. ビュー作成

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // ビュー作成
        //=====================================================================

        /// シェーダーリソースビュー作成
        virtual TRefCountPtr<IRHIShaderResourceView> CreateShaderResourceView(
            IRHIResource* resource,
            const RHISRVDesc& desc) = 0;

        /// アンオーダードアクセスビュー作成
        virtual TRefCountPtr<IRHIUnorderedAccessView> CreateUnorderedAccessView(
            IRHIResource* resource,
            const RHIUAVDesc& desc) = 0;

        /// レンダーターゲットビュー作成
        virtual TRefCountPtr<IRHIRenderTargetView> CreateRenderTargetView(
            IRHITexture* texture,
            const RHIRTVDesc& desc) = 0;

        /// デプスステンシルビュー作成
        virtual TRefCountPtr<IRHIDepthStencilView> CreateDepthStencilView(
            IRHITexture* texture,
            const RHIDSVDesc& desc) = 0;

        /// 定数バッファビュー作成
        virtual TRefCountPtr<IRHIConstantBufferView> CreateConstantBufferView(
            IRHIBuffer* buffer,
            const RHICBVDesc& desc) = 0;
    };
}
```

- [ ] CreateShaderResourceView
- [ ] CreateUnorderedAccessView
- [ ] CreateRenderTargetView
- [ ] CreateDepthStencilView

### 3. シェーダー/パイプライン作成

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // シェーダー作成
        //=====================================================================

        /// シェーダー作成
        /// @param desc シェーダー記述情報（バイトコード含む）。
        virtual TRefCountPtr<IRHIShader> CreateShader(
            const RHIShaderDesc& desc) = 0;

        //=====================================================================
        // パイプラインステート作成
        //=====================================================================

        /// グラフィックスパイプラインステート作成
        virtual TRefCountPtr<IRHIGraphicsPipelineState> CreateGraphicsPipelineState(
            const RHIGraphicsPipelineStateDesc& desc) = 0;

        /// コンピュートパイプラインステート作成
        virtual TRefCountPtr<IRHIComputePipelineState> CreateComputePipelineState(
            const RHIComputePipelineStateDesc& desc) = 0;

        //=====================================================================
        // ルートシグネチャ作成
        //=====================================================================

        /// ルートシグネチャ作成
        virtual TRefCountPtr<IRHIRootSignature> CreateRootSignature(
            const RHIRootSignatureDesc& desc) = 0;
    };
}
```

- [ ] CreateShader
- [ ] CreateGraphicsPipelineState
- [ ] CreateComputePipelineState
- [ ] CreateRootSignature

### 4. サンプラー/ステート作成

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // サンプラー作成
        //=====================================================================

        /// サンプラー作成
        virtual TRefCountPtr<IRHISampler> CreateSampler(
            const RHISamplerDesc& desc) = 0;

        //=====================================================================
        // 同期オブジェクト作成
        //=====================================================================

        /// フェンス作成
        /// @param initialValue 初期フェンス値
        virtual TRefCountPtr<IRHIFence> CreateFence(uint64 initialValue = 0) = 0;
    };
}
```

- [ ] CreateSampler
- [ ] CreateFence

### 5. スワップチェーン/クエリ作成

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // スワップチェーン作成
        //=====================================================================

        /// スワップチェーン作成
        /// @param desc スワップチェーン記述情報
        /// @param windowHandle ネイティブウィンドウハンドル
        virtual TRefCountPtr<IRHISwapChain> CreateSwapChain(
            const RHISwapChainDesc& desc,
            void* windowHandle) = 0;

        //=====================================================================
        // クエリプール作成
        //=====================================================================

        /// クエリプール作成
        virtual TRefCountPtr<IRHIQueryHeap> CreateQueryHeap(
            const RHIQueryHeapDesc& desc) = 0;

        //=====================================================================
        // ディスクリプタヒープ作成
        //=====================================================================

        /// ディスクリプタヒープ作成
        virtual TRefCountPtr<IRHIDescriptorHeap> CreateDescriptorHeap(
            const RHIDescriptorHeapDesc& desc) = 0;
    };
}
```

- [ ] CreateSwapChain
- [ ] CreateQueryHeap
- [ ] CreateDescriptorHeap

## 検証方法

- [ ] 全ファクトリメソッドの定義
- [ ] Desc構造体の前方宣言/定義
- [ ] 戻り値型の整合性
