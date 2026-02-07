# 01-23: IRHIDeviceディスクリプタ管理

## 目的

IRHIDeviceのディスクリプタ管理インターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (4. Device - ディスクリプタ管理
- docs/RHI/RHI_Implementation_Guide_Part3.md (ディスクリプタ管理

## 変更対象ファイル

更新:
- `Source/Engine/RHI/Public/IRHIDevice.h`

## TODO

### 1. ディスクリプタヒープのマネージャー

```cpp
namespace NS::RHI
{
    /// 前方宣言
    class IRHIDescriptorHeapManager;
    class IRHIOnlineDescriptorManager;
    class IRHIOfflineDescriptorManager;
    class IRHISamplerHeap;

    class IRHIDevice
    {
    public:
        //=====================================================================
        // ディスクリプタヒープ管理
        //=====================================================================

        /// ディスクリプタヒープのマネージャー取得
        /// ヒープの体の管理を担う。
        virtual IRHIDescriptorHeapManager* GetDescriptorHeapManager() = 0;
    };
}
```

- [ ] GetDescriptorHeapManager

### 2. オンラインディスクリプタ

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // オンライン（GPU可視）デスクリプタ
        //=====================================================================

        /// オンラインディスクリプタマネージャー取得
        /// シェーダーから可視なディスクリプタの管理
        /// フレームごとのブロックベース割り当て
        virtual IRHIOnlineDescriptorManager* GetOnlineDescriptorManager() = 0;

        /// オンラインディスクリプタを割り当て
        /// @param heapType ヒープタイプ（CBV_SRV_UAV または Sampler）。
        /// @param count 割り当て数
        /// @return 割り当てられたハンドル（先頭）。
        virtual RHIDescriptorHandle AllocateOnlineDescriptors(
            ERHIDescriptorHeapType heapType,
            uint32 count) = 0;
    };
}
```

- [ ] GetOnlineDescriptorManager
- [ ] AllocateOnlineDescriptors

### 3. オフラインディスクリプタ

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // オフライン（CPU専用）デスクリプタ
        //=====================================================================

        /// オフラインディスクリプタマネージャー取得
        /// CPU専用ディスクリプタ（ステージング用）。
        /// @param heapType ヒープタイプ
        virtual IRHIOfflineDescriptorManager* GetOfflineDescriptorManager(
            ERHIDescriptorHeapType heapType) = 0;

        /// オフラインディスクリプタを割り当て
        /// @param heapType ヒープタイプ
        /// @return 割り当てられたハンドル
        virtual RHICPUDescriptorHandle AllocateOfflineDescriptor(
            ERHIDescriptorHeapType heapType) = 0;

        /// オフラインディスクリプタを解放
        /// @param heapType ヒープタイプ
        /// @param handle 解放するハンドル
        virtual void FreeOfflineDescriptor(
            ERHIDescriptorHeapType heapType,
            RHICPUDescriptorHandle handle) = 0;
    };
}
```

- [ ] GetOfflineDescriptorManager
- [ ] AllocateOfflineDescriptor / FreeOfflineDescriptor

### 4. サンプラーヒープ

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // グローバルサンプラーヒープ
        //=====================================================================

        /// グローバルサンプラーヒープ取得
        /// サンプラーはグローバルヒープで管理
        virtual IRHISamplerHeap* GetGlobalSamplerHeap() = 0;

        /// サンプラーディスクリプタを割り当て
        /// @param sampler サンプラーオブジェクト
        /// @return 割り当てられたハンドル
        virtual RHIDescriptorHandle AllocateSamplerDescriptor(
            IRHISampler* sampler) = 0;
    };
}
```

- [ ] GetGlobalSamplerHeap
- [ ] AllocateSamplerDescriptor

### 5. ディスクリプタ操作

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // ディスクリプタ操作
        //=====================================================================

        /// ディスクリプタコピー
        /// @param dstHandle コピー先
        /// @param srcHandle コピー先
        /// @param heapType ヒープタイプ
        virtual void CopyDescriptor(
            RHICPUDescriptorHandle dstHandle,
            RHICPUDescriptorHandle srcHandle,
            ERHIDescriptorHeapType heapType) = 0;

        /// ディスクリプタ複数コピー
        /// @param dstStart コピー先開始
        /// @param srcStart コピー元開始
        /// @param count コピー数
        /// @param heapType ヒープタイプ
        virtual void CopyDescriptors(
            RHICPUDescriptorHandle dstStart,
            RHICPUDescriptorHandle srcStart,
            uint32 count,
            ERHIDescriptorHeapType heapType) = 0;

        /// ディスクリプタインクリメントサイズ取得
        /// @param heapType ヒープタイプ
        /// @return 1ディスクリプタあたりのバイト数
        virtual uint32 GetDescriptorIncrementSize(
            ERHIDescriptorHeapType heapType) const = 0;

        //=====================================================================
        // Bindless（対応時）。
        //=====================================================================

        /// Bindlessがサポートされているか
        virtual bool SupportsBindless() const = 0;

        /// BindlessSRVインデックスを割り当て
        /// @param view 登録するSRV
        /// @return Bindlessインデックス
        virtual BindlessSRVIndex AllocateBindlessSRV(
            IRHIShaderResourceView* view) = 0;

        /// BindlessUAVインデックスを割り当て
        virtual BindlessUAVIndex AllocateBindlessUAV(
            IRHIUnorderedAccessView* view) = 0;

        /// Bindlessインデックスを解放
        virtual void FreeBindlessSRV(BindlessSRVIndex index) = 0;
        virtual void FreeBindlessUAV(BindlessUAVIndex index) = 0;
    };
}
```

- [ ] CopyDescriptor / CopyDescriptors
- [ ] GetDescriptorIncrementSize
- [ ] Bindless対応

## 検証方法

- [ ] ディスクリプタ割り当て/解放の正確性
- [ ] コピー操作の整合性
- [ ] Bindlessインデックスの一意性
