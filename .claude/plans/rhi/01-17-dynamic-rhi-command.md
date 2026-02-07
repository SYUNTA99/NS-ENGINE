# 01-17: IDynamicRHIコマンド実行管理

## 目的

IDynamicRHIのコマンドコンテキスト実行管理メソッドを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (コマンド実行管理)
- docs/RHI/RHI_Implementation_Guide_Part2.md (コマンドコンテキスト)

## 変更対象ファイル

更新:
- `Source/Engine/RHI/Public/IDynamicRHI.h`

## TODO

### 1. コンテキスト取得

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // コマンドコンテキスト取得
        //=====================================================================

        /// デフォルトコンテキスト取得
        /// レンダースレッド専用の即時実行コンテキスト
        /// @return デフォルトコマンドコンテキスト
        virtual IRHICommandContext* GetDefaultContext() = 0;

        /// パイプライン別コンテキスト取得
        /// @param pipeline Graphics または AsyncCompute
        /// @return コマンドコンテキスト
        virtual IRHICommandContext* GetCommandContext(ERHIPipeline pipeline) = 0;

        /// コンピュートコンテキスト取得（AsyncCompute用）。
        virtual IRHIComputeContext* GetComputeContext() = 0;
    };
}
```

- [ ] GetDefaultContext
- [ ] GetCommandContext
- [ ] GetComputeContext

### 2. コマンドリスト管理

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // コマンドリスト取得解放
        //=====================================================================

        /// コマンドアロケーター取得
        /// @param queueType キュータイプ
        /// @return コマンドアロケーター（プールから）。
        virtual IRHICommandAllocator* ObtainCommandAllocator(
            ERHIQueueType queueType) = 0;

        /// コマンドアロケーター解放
        /// GPU完了後に再利用可能になる
        /// @param allocator 解放するアロケーター
        /// @param fence 完了時フェンス
        /// @param fenceValue 完了時フェンス値
        virtual void ReleaseCommandAllocator(
            IRHICommandAllocator* allocator,
            IRHIFence* fence,
            uint64 fenceValue) = 0;

        /// コマンドリスト取得
        /// @param allocator 使用するアロケーター
        /// @return コマンドリスト（プールから）。
        virtual IRHICommandList* ObtainCommandList(
            IRHICommandAllocator* allocator) = 0;

        /// コマンドリスト解放
        /// Close後に再利用可能
        virtual void ReleaseCommandList(IRHICommandList* commandList) = 0;
    };
}
```

- [ ] ObtainCommandAllocator / ReleaseCommandAllocator
- [ ] ObtainCommandList / ReleaseCommandList

### 3. コンテキスト終了コマンドリスト化

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // コンテキスト終了
        //=====================================================================

        /// コンテキストを終了しコマンドリスト化
        /// @param context 終了するコンテキスト
        /// @return 記録されたコマンドリスト
        virtual IRHICommandList* FinalizeContext(IRHICommandContext* context) = 0;

        /// コンテキストをリセット（再利用準備）。
        /// @param context リセットするコンテキスト
        virtual void ResetContext(IRHICommandContext* context) = 0;
    };
}
```

- [ ] FinalizeContext
- [ ] ResetContext

### 4. コマンド送信

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // コマンド送信
        //=====================================================================

        /// コマンドリストをGPUに送信
        /// @param queueType 送信先キュータイプ
        /// @param commandLists 送信するコマンドリストの分
        /// @param count コマンドリスト数
        virtual void SubmitCommandLists(
            ERHIQueueType queueType,
            IRHICommandList* const* commandLists,
            uint32 count) = 0;

        /// 単一コマンドリスト送信（便利関数）。
        void SubmitCommandList(ERHIQueueType queueType, IRHICommandList* commandList)
        {
            SubmitCommandLists(queueType, &commandList, 1);
        }

        /// 全コマンドの完了を待つ。
        /// @note GPUアイドルを引き起こすので注愁
        virtual void FlushCommands() = 0;

        /// 指定キューのコマンド完了を待つ。
        virtual void FlushQueue(ERHIQueueType queueType) = 0;
    };
}
```

- [ ] SubmitCommandLists
- [ ] FlushCommands / FlushQueue

### 5. 同期

```cpp
namespace NS::RHI
{
    class IDynamicRHI
    {
    public:
        //=====================================================================
        // GPU同期
        //=====================================================================

        /// フェンスシグナル
        /// @param queue シグナルを発行するキュー
        /// @param fence シグナルするフェンス
        /// @param value シグナル値
        virtual void SignalFence(
            IRHIQueue* queue,
            IRHIFence* fence,
            uint64 value) = 0;

        /// フェンス待ち（CPU側）。
        /// @param queue 待機するキュー
        /// @param fence 待機するフェンス
        /// @param value 待機する値
        virtual void WaitFence(
            IRHIQueue* queue,
            IRHIFence* fence,
            uint64 value) = 0;

        /// フェンス待ち（CPU側）。
        /// @param fence 待機するフェンス
        /// @param value 待機する値
        /// @param timeoutMs タイムアウト（ミリ秒、UINT64_MAX=無限）
        /// @return 完了したらtrue、タイムアウトでfalse
        virtual bool WaitForFence(
            IRHIFence* fence,
            uint64 value,
            uint64 timeoutMs = UINT64_MAX) = 0;

        //=====================================================================
        // フレーム同期
        //=====================================================================

        /// フレーム開始
        virtual void BeginFrame() {}

        /// 現在のフレーム番号取得
        virtual uint64 GetCurrentFrameNumber() const = 0;
    };
}
```

- [ ] SignalFence / WaitFence
- [ ] WaitForFence (CPU待ち。
- [ ] BeginFrame / GetCurrentFrameNumber

## 検証方法

- [ ] コマンド実行フローの整合性
- [ ] 同期プリミティブの正確性
- [ ] デッドロック防止
