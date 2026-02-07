# 21-03: Indirect Dispatch

## 概要

GPU駆動のコンピュートシェーダーディスパッチ、
スレッドグループ数をGPUバッファから読み取り、

## ファイル

- `Source/Engine/RHI/Public/IRHICommandContext.h` (拡張)

## 依存

- 21-01-indirect-arguments.md (RHIDispatchArguments)
- 02-02-compute-context.md (IRHIComputeContext)

## 定義

```cpp
namespace NS::RHI
{

// IRHIComputeContextへの追加メソッド

class IRHIComputeContext : public IRHICommandContext
{
public:
    // ... 既存メソッド ...

    // ════════════════════════════════════════════════════════════════
    // Indirect Dispatch
    // ════════════════════════════════════════════════════════════════

    /// 間接ディスパッチ
    /// @param argumentBuffer 引数バッファ（RHIDispatchArguments）。
    /// @param argumentOffset バッファ内のオフセット
    virtual void DispatchIndirect(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset = 0) = 0;

    /// 複数回間接ディスパッチ
    /// @param argumentBuffer 引数バッファ
    /// @param argumentOffset バッファ内のオフセット
    /// @param dispatchCount ディスパッチ回数
    /// @param stride 引数間のストライド（=sizeof(RHIDispatchArguments)）。
    virtual void DispatchIndirectMulti(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset,
        uint32 dispatchCount,
        uint32 stride = 0) = 0;
};

// ════════════════════════════════════════════════════════════════
// GPUカウンティング用ヘルパー
// ════════════════════════════════════════════════════════════════

/// GPU駆動ディスパッチヘルパー。
/// コンピュートシェーダーでスレッドグループ数を計算し、即座にディスパッチ
class RHIGPUDrivenDispatcher
{
public:
    explicit RHIGPUDrivenDispatcher(IRHIDevice* device);

    /// 引数バッファを設定
    void SetArgumentBuffer(RHIBufferRef buffer, uint64 offset = 0);

    /// アイテム数からスレッドグループ数を計算するシェーダーをディスパッチ
    /// argumentBuffer.x = (itemCount + groupSize - 1) / groupSize
    void DispatchWithItemCount(
        IRHIComputeContext* context,
        IRHIBuffer* itemCountBuffer,
        uint64 itemCountOffset,
        uint32 groupSizeX);

    /// 計算された引数で間接ディスパッチ
    void ExecuteIndirect(IRHIComputeContext* context);

private:
    RHIBufferRef m_argumentBuffer;
    uint64 m_argumentOffset = 0;
    RHIComputePSORef m_countToGroupsPSO;
};

} // namespace NS::RHI
```

## 使用例

```cpp
// パーティクルシミュレーション
// GPUでアクティブパーティクル数を計算し、その数に基づいシミュレーション

// 1. カウントシェーダーでアクティブ数を計算
context->SetPSO(countActivePSO);
context->Dispatch(particleGroups, 1, 1);

// 2. Indirect引数を生成（別のコンピュートシェーダーで）。
context->SetPSO(prepareIndirectPSO);
context->Dispatch(1, 1, 1);

// 3. シミュレーションを間接ディスパッチ
context->SetPSO(simulatePSO);
context->DispatchIndirect(indirectArgsBuffer, 0);
```

## 検証

- [ ] 単一間接ディスパッチ
- [ ] Multi間接ディスパッチ
- [ ] ゼログループ数の安全性
