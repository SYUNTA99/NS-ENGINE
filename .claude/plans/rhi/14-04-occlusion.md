# 14-04: オクルージョン

## 目的

オクルージョンクエリとプレディケーションを定義する。

## 参照ドキュメント

- 14-01-query-types.md (ERHIQueryType::Occlusion)
- 14-02-query-pool.md (IRHIQueryHeap)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIOcclusion.h`

## TODO

### 1. オクルージョンクエリ結果

```cpp
#pragma once

#include "RHIQuery.h"

namespace NS::RHI
{
    /// オクルージョンクエリ結果（拡張）。
    struct RHI_API RHIOcclusionResult
    {
        /// 可視サンプル数
        uint64 samplesPassed = 0;

        /// 有効か
        bool valid = false;

        /// 可視か
        bool IsVisible() const { return valid && samplesPassed > 0; }

        /// 可視度（0.0〜1.0、参照サンプル数が必要）。
        float GetVisibility(uint64 referenceSamples) const {
            if (!valid || referenceSamples == 0) return 0.0f;
            return static_cast<float>(samplesPassed) / referenceSamples;
        }
    };
}
```

- [ ] RHIOcclusionResult 構造体

### 2. オクルージョンクエリマネージャー

```cpp
namespace NS::RHI
{
    /// オクルージョンクエリID
    struct RHI_API RHIOcclusionQueryId
    {
        uint32 index = ~0u;

        bool IsValid() const { return index != ~0u; }

        static RHIOcclusionQueryId Invalid() { return {}; }
    };

    /// オクルージョンクエリマネージャー
    class RHI_API RHIOcclusionQueryManager
    {
    public:
        RHIOcclusionQueryManager() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            uint32 maxQueries = 4096,
            uint32 numBufferedFrames = 3,
            bool useBinaryOcclusion = true);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame(IRHICommandContext* context);

        //=====================================================================
        // クエリ操作
        //=====================================================================

        /// クエリ開始
        RHIOcclusionQueryId BeginQuery(IRHICommandContext* context);

        /// クエリ終了
        void EndQuery(IRHICommandContext* context, RHIOcclusionQueryId id);

        //=====================================================================
        // 結果取得
        //=====================================================================

        /// 結果準備完了後
        bool AreResultsReady() const;

        /// 結果取得
        RHIOcclusionResult GetResult(RHIOcclusionQueryId id) const;

        /// 可視か（バイナリ結果）。
        bool IsVisible(RHIOcclusionQueryId id) const;

        //=====================================================================
        // 情報
        //=====================================================================

        /// 現在フレームの使用クエリ数
        uint32 GetUsedQueryCount() const { return m_currentQueryCount; }

        /// 最大クエリ数
        uint32 GetMaxQueryCount() const { return m_maxQueries; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIQueryAllocator m_queryAllocator;
        bool m_useBinaryOcclusion = true;

        uint32 m_maxQueries = 0;
        uint32 m_currentQueryCount = 0;

        RHIOcclusionResult* m_results = nullptr;
        uint32 m_resultCount = 0;
    };
}
```

- [ ] RHIOcclusionQueryId 構造体
- [ ] RHIOcclusionQueryManager クラス

### 3. プレディケーション

```cpp
namespace NS::RHI
{
    /// プレディケーション操作（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// プレディケーション設定
        /// @param buffer クエリ結果バッファ
        /// @param offset バッファ内のオフセット
        /// @param operation 比較操作
        virtual void SetPredication(
            IRHIBuffer* buffer,
            uint64 offset,
            ERHIPredicationOp operation) = 0;

        /// プレディケーション解除
        void ClearPredication() {
            SetPredication(nullptr, 0, ERHIPredicationOp::EqualZero);
        }
    };

    /// プレディケーション操作
    enum class ERHIPredicationOp : uint8
    {
        /// 値がゼロなら描画をスキップ。
        EqualZero,

        /// 値が非ゼロなら描画をスキップ。
        NotEqualZero,
    };
}
```

- [ ] SetPredication
- [ ] ERHIPredicationOp 列挙型

### 4. 条件付きレンダリングヘルパー

```cpp
namespace NS::RHI
{
    /// 条件付きレンダリング
    /// オクルージョン結果に基づい描画をスキップ。
    class RHI_API RHIConditionalRendering
    {
    public:
        RHIConditionalRendering() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            RHIOcclusionQueryManager* occlusionManager);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame(IRHICommandContext* context);

        //=====================================================================
        // オブジェクト登録
        //=====================================================================

        /// オブジェクト登録
        /// @param objectId ユーザー定義オブジェクトID
        /// @return 登録に成功したか
        bool RegisterObject(uint32 objectId);

        /// オブジェクト登録解除
        void UnregisterObject(uint32 objectId);

        //=====================================================================
        // オクルージョンテスト
        //=====================================================================

        /// オブジェクトのオクルージョンテスト開始
        void BeginOcclusionTest(
            IRHICommandContext* context,
            uint32 objectId);

        /// オクルージョンテスト終了
        void EndOcclusionTest(
            IRHICommandContext* context,
            uint32 objectId);

        //=====================================================================
        // 条件付き描画
        //=====================================================================

        /// オブジェクトが可視の場合のみ描画開始
        /// @return 描画すべきか（前フレームの結果に基づく）
        bool BeginConditionalDraw(
            IRHICommandContext* context,
            uint32 objectId);

        /// 条件付き描画終了
        void EndConditionalDraw(IRHICommandContext* context);

        //=====================================================================
        // 結果取得
        //=====================================================================

        /// オブジェクトが可視か
        bool IsObjectVisible(uint32 objectId) const;

    private:
        IRHIDevice* m_device = nullptr;
        RHIOcclusionQueryManager* m_occlusionManager = nullptr;

        struct ObjectData {
            RHIOcclusionQueryId queryId;
            bool visible;
            bool tested;
        };
        // objectId -> ObjectData マッピング
    };
}
```

- [ ] RHIConditionalRendering クラス

### 5. 階層オクルージョンカリング

```cpp
namespace NS::RHI
{
    /// HiZバッファ
    /// 階層的オクルージョンカリング用
    class RHI_API RHIHiZBuffer
    {
    public:
        RHIHiZBuffer() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            uint32 width,
            uint32 height);

        /// シャットダウン
        void Shutdown();

        /// リサイズ
        bool Resize(uint32 width, uint32 height);

        //=====================================================================
        // HiZ生成。
        //=====================================================================

        /// デプスバッファからHiZを生成
        void Generate(
            IRHICommandContext* context,
            IRHITexture* depthBuffer);

        //=====================================================================
        // テクスチャ取得
        //=====================================================================

        /// HiZテクスチャ取得
        IRHITexture* GetHiZTexture() const { return m_hiZTexture; }

        /// ミップレベル数取得
        uint32 GetMipLevelCount() const { return m_mipCount; }

        /// SRV取得
        IRHIShaderResourceView* GetSRV() const { return m_srv; }

    private:
        IRHIDevice* m_device = nullptr;
        RHITextureRef m_hiZTexture;
        RHIShaderResourceViewRef m_srv;
        uint32 m_width = 0;
        uint32 m_height = 0;
        uint32 m_mipCount = 0;

        // HiZ生成用コンピュートシェーダー
        IRHIComputePipelineState* m_hiZGenPSO = nullptr;
    };
}
```

- [ ] RHIHiZBuffer クラス

## 検証方法

- [ ] オクルージョンクエリの動作
- [ ] プレディケーションの効果
- [ ] 条件付きレンダリング
- [ ] HiZ生成。
