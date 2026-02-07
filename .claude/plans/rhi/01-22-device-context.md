# 01-22: IRHIDeviceコンテキスト管理

## 目的

IRHIDeviceのコマンドコンテキストアロケーター管理インターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (4. Device - コンテキスト管理
- docs/RHI/RHI_Implementation_Guide_Part2.md (コマンドコンテキスト

## 変更対象ファイル

更新:
- `Source/Engine/RHI/Public/IRHIDevice.h`

## TODO

### 1. 即時コンテキスト

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // 即時コンテキスト
        //=====================================================================

        /// 即時コンテキスト取得
        /// レンダースレッド専用のデフォルトコンテキスト
        /// @return 即時コマンドコンテキスト
        virtual IRHICommandContext* GetImmediateContext() = 0;

        /// グラフィックス即時コンテキスト（キャスト）
        IRHICommandContext* GetImmediateGraphicsContext() {
            return GetImmediateContext();
        }
    };
}
```

- [ ] GetImmediateContext

### 2. コンテキストプール

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // コンテキストプール
        // 並列コマンドリスト記録用
        //=====================================================================

        /// コンテキスト取得（プールから）。
        /// @param queueType 対象キュータイプ
        /// @return コマンドコンテキスト
        virtual IRHICommandContext* ObtainContext(ERHIQueueType queueType) = 0;

        /// コンピュートコンテキスト取得
        virtual IRHIComputeContext* ObtainComputeContext() = 0;

        /// コンテキスト解放（プールに返却）。
        /// @param context 解放するコンテキスト
        virtual void ReleaseContext(IRHICommandContext* context) = 0;

        /// コンピュートコンテキスト解放
        virtual void ReleaseContext(IRHIComputeContext* context) = 0;
    };
}
```

- [ ] ObtainContext / ReleaseContext
- [ ] コンピュートコンテキスト対応

### 3. コマンドアロケーター

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // コマンドアロケーター管理
        //=====================================================================

        /// コマンドアロケーター取得（プールから）。
        /// @param queueType 対象キュータイプ
        /// @return コマンドアロケーター
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

        /// コマンドアロケーターを即座に解放
        /// @note GPU同期が完了していることを呼び出しのが保証
        virtual void ReleaseCommandAllocatorImmediate(
            IRHICommandAllocator* allocator) = 0;
    };
}
```

- [ ] ObtainCommandAllocator
- [ ] ReleaseCommandAllocator

### 4. コマンドリスト

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // コマンドリスト管理
        //=====================================================================

        /// コマンドリスト取得（プールから）。
        /// @param allocator 使用するアロケーター
        /// @param pipelineState 初期パイプラインステート（nullptrで未設定）
        /// @return コマンドリスト
        virtual IRHICommandList* ObtainCommandList(
            IRHICommandAllocator* allocator,
            IRHIPipelineState* pipelineState = nullptr) = 0;

        /// コマンドリスト解放
        /// Close後に再利用可能
        /// @param commandList 解放するコマンドリスト
        virtual void ReleaseCommandList(IRHICommandList* commandList) = 0;
    };
}
```

- [ ] ObtainCommandList
- [ ] ReleaseCommandList

### 5. コンテキスト終了リセット

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // コンテキスト終了
        //=====================================================================

        /// コンテキストを終了しコマンドリスト化
        /// @param context 終了するコンテキスト
        /// @return 記録されたコマンドリスト
        virtual IRHICommandList* FinalizeContext(
            IRHICommandContext* context) = 0;

        /// コンピュートコンテキストを終了
        virtual IRHICommandList* FinalizeContext(
            IRHIComputeContext* context) = 0;

        /// コンテキストをリセット（再利用準備）。
        /// @param context リセットするコンテキスト
        /// @param allocator 新しいアロケーター
        virtual void ResetContext(
            IRHICommandContext* context,
            IRHICommandAllocator* allocator) = 0;

        /// コンピュートコンテキストをリセット
        virtual void ResetContext(
            IRHIComputeContext* context,
            IRHICommandAllocator* allocator) = 0;
    };
}
```

- [ ] FinalizeContext
- [ ] ResetContext

## 検証方法

- [ ] プール管理の正確性
- [ ] アロケーターライフサイクル
- [ ] コンテキストの利用の安全性
