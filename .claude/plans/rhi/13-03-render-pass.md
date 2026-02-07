# 13-03: レンダーパス実行

## 目的

レンダーパスの開始と終了操作を定義する。

## 参照ドキュメント

- 13-01-render-pass-desc.md (RHIRenderPassDesc)
- 02-03-graphics-context.md (IRHIGraphicsContext)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIRenderPass.h` (部分

## TODO

### 1. レンダーパス開始終了

```cpp
namespace NS::RHI
{
    /// レンダーパス操作（RHIGraphicsContextに追加）。
    class IRHIGraphicsContext
    {
    public:
        /// レンダーパス開始
        /// @param desc レンダーパス記述
        virtual void BeginRenderPass(const RHIRenderPassDesc& desc) = 0;

        /// レンダーパス終了
        virtual void EndRenderPass() = 0;

        /// レンダーパス中か
        virtual bool IsInRenderPass() const = 0;

        /// 現在のレンダーパス記述取得
        virtual const RHIRenderPassDesc* GetCurrentRenderPassDesc() const = 0;
    };
}
```

- [ ] BeginRenderPass
- [ ] EndRenderPass

### 2. スコープレンダーパス

```cpp
namespace NS::RHI
{
    /// スコープレンダーパス（AII）。
    class RHI_API RHIScopedRenderPass
    {
    public:
        RHIScopedRenderPass(IRHIGraphicsContext* context, const RHIRenderPassDesc& desc)
            : m_context(context)
        {
            if (m_context) {
                m_context->BeginRenderPass(desc);
            }
        }

        ~RHIScopedRenderPass() {
            if (m_context) {
                m_context->EndRenderPass();
            }
        }

        NS_DISALLOW_COPY(RHIScopedRenderPass);

    public:
        /// ムーチ
        RHIScopedRenderPass(RHIScopedRenderPass&& other) noexcept
            : m_context(other.m_context)
        {
            other.m_context = nullptr;
        }

    private:
        IRHIGraphicsContext* m_context;
    };

    /// スコープレンダーパスマクロ
    #define RHI_SCOPED_RENDER_PASS(context, desc) \
        NS::RHI::RHIScopedRenderPass NS_MACRO_CONCATENATE(_rhiRenderPass, __LINE__)(context, desc)
}
```

- [ ] RHIScopedRenderPass クラス

### 3. サブパス

```cpp
namespace NS::RHI
{
    /// サブパス記述
    struct RHI_API RHISubpassDesc
    {
        /// 入力アタッチメントインデックス
        const uint32* inputAttachments = nullptr;
        uint32 inputAttachmentCount = 0;

        /// 出力アタッチメントインデックス
        const uint32* colorAttachments = nullptr;
        uint32 colorAttachmentCount = 0;

        /// リゾルブアタッチメントインデックス
        const uint32* resolveAttachments = nullptr;

        /// 保持アタッチメントインデックス
        const uint32* preserveAttachments = nullptr;
        uint32 preserveAttachmentCount = 0;

        /// デプスステンシルアタッチメント
        int32 depthStencilAttachment = -1;  // -1で未使用
    };

    /// サブパス依存関係
    struct RHI_API RHISubpassDependency
    {
        /// ソースサブパス（ExternalSubpassで外部）。
        uint32 srcSubpass = 0;

        /// デストサブパス
        uint32 dstSubpass = 0;

        /// ソースステージマスク
        ERHIPipelineStageFlags srcStageMask = ERHIPipelineStageFlags::AllCommands;

        /// デストステージマスク
        ERHIPipelineStageFlags dstStageMask = ERHIPipelineStageFlags::AllCommands;

        /// ソースアクセスマスク
        ERHIAccessFlags srcAccessMask = ERHIAccessFlags::None;

        /// デストアクセスマスク
        ERHIAccessFlags dstAccessMask = ERHIAccessFlags::None;

        /// 外部サブパス定数
        static constexpr uint32 kExternalSubpass = ~0u;
    };

    /// サブパス操作（RHIGraphicsContextに追加）。
    class IRHIGraphicsContext
    {
    public:
        /// 次のサブパスへ進む
        virtual void NextSubpass() = 0;

        /// 現在のサブパスインデックス取得
        virtual uint32 GetCurrentSubpassIndex() const = 0;
    };
}
```

- [ ] RHISubpassDesc 構造体
- [ ] RHISubpassDependency 構造体
- [ ] NextSubpass

### 4. レガシーRTセット操作

```cpp
namespace NS::RHI
{
    /// レガシーRT設定（BeginRenderPassを使用しない合）
    /// IRHIGraphicsContextに追加
    class IRHIGraphicsContext
    {
    public:
        /// レンダーターゲット設定（レガシー）。
        virtual void SetRenderTargets(
            uint32 numRenderTargets,
            IRHIRenderTargetView* const* renderTargets,
            IRHIDepthStencilView* depthStencil = nullptr) = 0;

        /// 単一RT設定（便利関数）。
        void SetRenderTarget(
            IRHIRenderTargetView* rt,
            IRHIDepthStencilView* ds = nullptr)
        {
            SetRenderTargets(rt ? 1 : 0, rt ? &rt : nullptr, ds);
        }

        /// RTクリア
        virtual void ClearRenderTarget(
            IRHIRenderTargetView* rtv,
            const float color[4]) = 0;

        /// デプスステンシルクリア
        virtual void ClearDepthStencil(
            IRHIDepthStencilView* dsv,
            ERHIClearFlags flags,
            float depth,
            uint8 stencil) = 0;

        /// デプスのみクリア
        void ClearDepth(IRHIDepthStencilView* dsv, float depth = 1.0f) {
            ClearDepthStencil(dsv, ERHIClearFlags::Depth, depth, 0);
        }

        /// ステンシルのみクリア
        void ClearStencil(IRHIDepthStencilView* dsv, uint8 stencil = 0) {
            ClearDepthStencil(dsv, ERHIClearFlags::Stencil, 0, stencil);
        }
    };

    /// クリアフラグ
    enum class ERHIClearFlags : uint8
    {
        None = 0,
        Depth = 1 << 0,
        Stencil = 1 << 1,
        DepthStencil = Depth | Stencil,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIClearFlags)
}
```

- [ ] SetRenderTargets（レガシー）。
- [ ] ClearRenderTarget / ClearDepthStencil

### 5. レンダーパス統計

```cpp
namespace NS::RHI
{
    /// レンダーパス統計
    struct RHI_API RHIRenderPassStatistics
    {
        /// 描画コール数
        uint32 drawCallCount = 0;

        /// 描画プリミティブ数
        uint64 primitiveCount = 0;

        /// 描画頂点数
        uint64 vertexCount = 0;

        /// インスタンス数
        uint64 instanceCount = 0;

        /// ディスパッチコール数（コンピュート）
        uint32 dispatchCount = 0;

        /// 状態変更数
        uint32 stateChangeCount = 0;

        /// リソースバリア数
        uint32 barrierCount = 0;
    };

    /// レンダーパス統計取得（RHIGraphicsContextに追加）。
    class IRHIGraphicsContext
    {
    public:
        /// レンダーパス統計取得
        virtual bool GetRenderPassStatistics(RHIRenderPassStatistics& outStats) const = 0;

        /// 統計リセット
        virtual void ResetStatistics() = 0;
    };
}
```

- [ ] RHIRenderPassStatistics 構造体
- [ ] GetRenderPassStatistics

## 検証方法

- [ ] レンダーパス開始終了
- [ ] ネストされたレンダーパスのエラー検出
- [ ] サブパス遷移
- [ ] 統計取得
