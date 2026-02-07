# 14-02: クエリプール

## 目的

クエリヒープ（プール）インターフェースを定義する。

## 参照ドキュメント

- 14-01-query-types.md (ERHIQueryType)
- 01-12-resource-base.md (IRHIResource)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIQuery.h` (部分

## TODO

### 1. IRHIQueryHeap インターフェース

```cpp
namespace NS::RHI
{
    /// クエリヒープ
    class RHI_API IRHIQueryHeap : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(QueryHeap)

        virtual ~IRHIQueryHeap() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// クエリタイプ取得
        virtual ERHIQueryType GetQueryType() const = 0;

        /// クエリ数取得
        virtual uint32 GetQueryCount() const = 0;

        /// パイプライン統計フラグ取得
        virtual ERHIPipelineStatisticsFlags GetPipelineStatisticsFlags() const = 0;

        //=====================================================================
        // 結果サイズ
        //=====================================================================

        /// 1クエリあたりの結果サイズ（バイト）
        virtual uint32 GetQueryResultSize() const = 0;

        /// アライメント取得
        virtual uint32 GetQueryResultAlignment() const = 0;
    };

    using RHIQueryHeapRef = TRefCountPtr<IRHIQueryHeap>;
}
```

- [ ] IRHIQueryHeap インターフェース

### 2. クエリヒープ作成

```cpp
namespace NS::RHI
{
    /// クエリヒープ作成（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// クエリヒープ作成
        virtual IRHIQueryHeap* CreateQueryHeap(
            const RHIQueryHeapDesc& desc,
            const char* debugName = nullptr) = 0;
    };
}
```

- [ ] CreateQueryHeap

### 3. クエリ操作

```cpp
namespace NS::RHI
{
    /// クエリ操作（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// クエリ開始
        virtual void BeginQuery(
            IRHIQueryHeap* queryHeap,
            uint32 queryIndex) = 0;

        /// クエリ終了
        virtual void EndQuery(
            IRHIQueryHeap* queryHeap,
            uint32 queryIndex) = 0;

        /// タイムスタンプ書き込み
        virtual void WriteTimestamp(
            IRHIQueryHeap* queryHeap,
            uint32 queryIndex) = 0;

        /// クエリ結果をバッファにコピー
        virtual void ResolveQueryData(
            IRHIQueryHeap* queryHeap,
            uint32 startIndex,
            uint32 queryCount,
            IRHIBuffer* destBuffer,
            uint64 destOffset,
            ERHIQueryFlags flags = ERHIQueryFlags::None) = 0;
    };
}
```

- [ ] BeginQuery / EndQuery
- [ ] WriteTimestamp
- [ ] ResolveQueryData

### 4. クエリ結果読み取り

```cpp
namespace NS::RHI
{
    /// クエリ結果読み取り（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// クエリデータ取得（CPU側）。
        /// @param queryHeap クエリヒープ
        /// @param startIndex 開始インデックス
        /// @param queryCount クエリ数
        /// @param destData 出力バッファ
        /// @param destStride 出力ストライド
        /// @param flags フラグ
        /// @return 成功したか
        virtual bool GetQueryData(
            IRHIQueryHeap* queryHeap,
            uint32 startIndex,
            uint32 queryCount,
            void* destData,
            uint32 destStride,
            ERHIQueryFlags flags = ERHIQueryFlags::None) = 0;

        /// タイムスタンプ周波数取得
        virtual uint64 GetTimestampFrequency() const = 0;

        /// GPUタイムスタンプ周波数値取得
        virtual bool GetTimestampCalibration(
            uint64& outGPUTimestamp,
            uint64& outCPUTimestamp) const = 0;
    };
}
```

- [ ] GetQueryData
- [ ] GetTimestampFrequency

### 5. クエリアロケーター

```cpp
namespace NS::RHI
{
    /// クエリアロケーション
    struct RHI_API RHIQueryAllocation
    {
        /// クエリヒープ
        IRHIQueryHeap* heap = nullptr;

        /// 開始インデックス
        uint32 startIndex = 0;

        /// クエリ数
        uint32 count = 0;

        /// 有効か
        bool IsValid() const { return heap != nullptr && count > 0; }
    };

    /// クエリアロケーター
    /// フレームごとのクエリ割り当てを管理
    class RHI_API RHIQueryAllocator
    {
    public:
        RHIQueryAllocator() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            ERHIQueryType type,
            uint32 queriesPerFrame,
            uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame(uint32 frameIndex);
        void EndFrame();

        //=====================================================================
        // クエリ割り当て
        //=====================================================================

        /// クエリ割り当て
        RHIQueryAllocation Allocate(uint32 count = 1);

        /// 利用可能クエリ数
        uint32 GetAvailableCount() const;

        //=====================================================================
        // 結果読み取り
        //=====================================================================

        /// 前フレームの結果準備完了後
        bool AreResultsReady(uint32 frameIndex) const;

        /// 結果バッファ取得
        IRHIBuffer* GetResultBuffer(uint32 frameIndex) const;

    private:
        IRHIDevice* m_device = nullptr;
        ERHIQueryType m_type = ERHIQueryType::Timestamp;

        struct FrameData {
            RHIQueryHeapRef heap;
            RHIBufferRef resultBuffer;
            uint32 allocatedCount;
            bool resolved;
        };
        FrameData* m_frameData = nullptr;
        uint32 m_numFrames = 0;
        uint32 m_currentFrame = 0;
        uint32 m_queriesPerFrame = 0;
    };
}
```

- [ ] RHIQueryAllocation 構造体
- [ ] RHIQueryAllocator クラス

## 検証方法

- [ ] クエリヒープ作成/破棄
- [ ] クエリ開始終了
- [ ] 結果の読み取り
- [ ] アロケーターの動作
