# 07-06: コンピュートPSO

## 目的

コンピュートパイプラインステートオブジェクト（PSO）を定義する。

## 参照ドキュメント

- 06-02-shader-interface.md (IRHIShader)
- 07-05-graphics-pso.md (PSO設計パターン)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIPipelineState.h` (部分

## TODO

### 1. コンピュートPSO記述

```cpp
namespace NS::RHI
{
    /// コンピュートパイプラインステート記述
    /// 作成時検証（デバッグビルドのみ）:
    /// 1. computeShader != nullptr
    /// 2. computeShader->GetFrequency() == EShaderFrequency::Compute
    /// 3. rootSignatureが非nullの場合、シェーダーの要求するバインディングを全て含むか検証
    /// 検証失敗時はRHI_ASSERTでフェイルファスト
    struct RHI_API RHIComputePipelineStateDesc
    {
        /// コンピュートシェーダー（必須）
        IRHIShader* computeShader = nullptr;

        /// ルートシグネチャ
        IRHIRootSignature* rootSignature = nullptr;

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

        RHIComputePipelineStateDesc& SetCS(IRHIShader* cs) { computeShader = cs; return *this; }
        RHIComputePipelineStateDesc& SetRootSignature(IRHIRootSignature* rs) { rootSignature = rs; return *this; }
        RHIComputePipelineStateDesc& SetNodeMask(uint32 mask) { nodeMask = mask; return *this; }

        /// シェーダーとルートシグネチャから作成
        static RHIComputePipelineStateDesc Create(
            IRHIShader* cs,
            IRHIRootSignature* rs = nullptr)
        {
            RHIComputePipelineStateDesc desc;
            desc.computeShader = cs;
            desc.rootSignature = rs;
            return desc;
        }
    };
    RHI_ENUM_CLASS_FLAGS(RHIComputePipelineStateDesc::Flags)
}
```

- [ ] RHIComputePipelineStateDesc 構造体
- [ ] ビルダーメソッド

### 2. IRHIComputePipelineState インターフェース

```cpp
namespace NS::RHI
{
    /// コンピュートパイプラインステート
    class RHI_API IRHIComputePipelineState : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(ComputePipelineState)

        virtual ~IRHIComputePipelineState() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// ルートシグネチャ取得
        virtual IRHIRootSignature* GetRootSignature() const = 0;

        /// コンピュートシェーダー取得
        virtual IRHIShader* GetComputeShader() const = 0;

        //=====================================================================
        // スレッドグループ情報
        //=====================================================================

        /// スレッドグループサイズ取得
        virtual void GetThreadGroupSize(
            uint32& outX, uint32& outY, uint32& outZ) const = 0;

        /// スレッドグループ合計スレッド数
        uint32 GetTotalThreadsPerGroup() const {
            uint32 x, y, z;
            GetThreadGroupSize(x, y, z);
            return x * y * z;
        }

        //=====================================================================
        // ディスパッチ計算ヘルパー。
        //=====================================================================

        /// 指定サイズをカバーするディスパッチグループ数計算
        void CalculateDispatchGroups(
            uint32 totalX, uint32 totalY, uint32 totalZ,
            uint32& outGroupsX, uint32& outGroupsY, uint32& outGroupsZ) const
        {
            uint32 threadX, threadY, threadZ;
            GetThreadGroupSize(threadX, threadY, threadZ);

            outGroupsX = (totalX + threadX - 1) / threadX;
            outGroupsY = (totalY + threadY - 1) / threadY;
            outGroupsZ = (totalZ + threadZ - 1) / threadZ;
        }

        /// 1Dディスパッチグループ数計算
        uint32 CalculateDispatchGroups1D(uint32 total) const {
            uint32 gx, gy, gz;
            CalculateDispatchGroups(total, 1, 1, gx, gy, gz);
            return gx;
        }

        /// 2Dディスパッチグループ数計算
        Extent2D CalculateDispatchGroups2D(uint32 width, uint32 height) const {
            uint32 gx, gy, gz;
            CalculateDispatchGroups(width, height, 1, gx, gy, gz);
            return Extent2D{gx, gy};
        }

        //=====================================================================
        // キャッシュ
        //=====================================================================

        /// キャッシュ済みバイナリ取得
        virtual RHIShaderBytecode GetCachedBlob() const = 0;
    };

    using RHIComputePipelineStateRef = TRefCountPtr<IRHIComputePipelineState>;
}
```

- [ ] IRHIComputePipelineState インターフェース
- [ ] スレッドグループ情報
- [ ] ディスパッチヘルパー。

### 3. コンピュートPSO作成

```cpp
namespace NS::RHI
{
    /// コンピュートPSO作成（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// コンピュートPSO作成
        virtual IRHIComputePipelineState* CreateComputePipelineState(
            const RHIComputePipelineStateDesc& desc,
            const char* debugName = nullptr) = 0;

        /// キャッシュ済みBlobからコンピュートPSO作成
        virtual IRHIComputePipelineState* CreateComputePipelineStateFromCache(
            const RHIComputePipelineStateDesc& desc,
            const RHIShaderBytecode& cachedBlob,
            const char* debugName = nullptr) = 0;

        /// シェーダーから直接コンピュートPSO作成（便利関数）。
        IRHIComputePipelineState* CreateComputePipelineState(
            IRHIShader* computeShader,
            IRHIRootSignature* rootSignature = nullptr,
            const char* debugName = nullptr)
        {
            return CreateComputePipelineState(
                RHIComputePipelineStateDesc::Create(computeShader, rootSignature),
                debugName);
        }
    };
}
```

- [ ] CreateComputePipelineState
- [ ] CreateComputePipelineStateFromCache

### 4. コンピュートPSO設定・ディスパッチ（

```cpp
namespace NS::RHI
{
    /// コンピュートPSO設定・ディスパッチ（RHIComputeContextに追加）。
    class IRHIComputeContext
    {
    public:
        /// コンピュートPSO設定
        virtual void SetComputePipelineState(IRHIComputePipelineState* pso) = 0;

        /// ディスパッチ
        virtual void Dispatch(
            uint32 threadGroupCountX,
            uint32 threadGroupCountY,
            uint32 threadGroupCountZ) = 0;

        /// 1Dディスパッチ
        void Dispatch1D(uint32 groupCountX) {
            Dispatch(groupCountX, 1, 1);
        }

        /// 2Dディスパッチ
        void Dispatch2D(uint32 groupCountX, uint32 groupCountY) {
            Dispatch(groupCountX, groupCountY, 1);
        }

        /// 間接ディスパッチ
        virtual void DispatchIndirect(
            IRHIBuffer* argsBuffer,
            uint64 argsOffset = 0) = 0;

        //=====================================================================
        // 便利メソッド
        //=====================================================================

        /// 指定要素数をカバーするディスパッチ（PSO設定後に使用）。
        void DispatchForElements(
            IRHIComputePipelineState* pso,
            uint32 totalX, uint32 totalY = 1, uint32 totalZ = 1)
        {
            uint32 gx, gy, gz;
            pso->CalculateDispatchGroups(totalX, totalY, totalZ, gx, gy, gz);
            Dispatch(gx, gy, gz);
        }

        /// テクスチャサイズでディスパッチ
        void DispatchForTexture(
            IRHIComputePipelineState* pso,
            IRHITexture* texture,
            uint32 mipLevel = 0)
        {
            uint32 width = CalculateMipSize(texture->GetWidth(), mipLevel);
            uint32 height = CalculateMipSize(texture->GetHeight(), mipLevel);
            DispatchForElements(pso, width, height);
        }
    };
}
```

- [ ] SetComputePipelineState
- [ ] Dispatch / DispatchIndirect
- [ ] 便利メソッド

### 5. 非同期コンピュート

```cpp
namespace NS::RHI
{
    /// 非同期コンピュートヘルパー。
    /// グラフィックスと並列実行するコンピュート処理を
    class RHI_API RHIAsyncComputeHelper
    {
    public:
        RHIAsyncComputeHelper() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        /// コンピュートキュー取得
        IRHIQueue* GetComputeQueue() const { return m_computeQueue; }

        //=====================================================================
        // 同期
        //=====================================================================

        /// グラフィックスからコンピュートへの同期ポイント挿入
        /// @return コンピュートので待機すべきフェンス値
        uint64 InsertGraphicsToComputeSync(IRHICommandContext* graphicsContext);

        /// コンピュートからグラフィックスへの同期ポイント挿入
        /// @return グラフィックス側で待機すべきフェンス値
        uint64 InsertComputeToGraphicsSync(IRHIComputeContext* computeContext);

        /// グラフィックス側でコンピュート完了を待つ。
        void WaitForComputeOnGraphics(
            IRHICommandContext* graphicsContext,
            uint64 computeFenceValue);

        /// コンピュートのでグラフィックス完了を待つ。
        void WaitForGraphicsOnCompute(
            IRHIComputeContext* computeContext,
            uint64 graphicsFenceValue);

        //=====================================================================
        // 便利メソッド
        //=====================================================================

        /// 単純な非同期コンピュートタスク実行
        /// @param setupFunc コンテキスト設定コールバック
        /// @return 完了確認用フェンス値
        using ComputeSetupFunc = std::function<void(IRHIComputeContext*)>;
        uint64 ExecuteAsync(ComputeSetupFunc setupFunc);

    private:
        IRHIDevice* m_device = nullptr;
        IRHIQueue* m_computeQueue = nullptr;
        RHIFenceRef m_computeFence;
        uint64 m_nextFenceValue = 1;
    };
}
```

- [ ] RHIAsyncComputeHelper クラス
- [ ] 同期ヘルパー
- [ ] ExecuteAsync

## 検証方法

- [ ] コンピュートPSO作成の正常動作
- [ ] ディスパッチの実行確認
- [ ] 間接ディスパッチの動作
- [ ] 非同期コンピュートの同期確認
