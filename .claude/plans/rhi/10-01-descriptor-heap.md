# 10-01: ディスクリプタヒープ

## 目的

ディスクリプタヒープのインターフェースと管理を定義する。

## 参照ドキュメント

- 01-04-types-descriptor.md (ディスクリプタハンドル)
- 01-07-enums-access.md (ERHIDescriptorType)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHIDescriptorHeap.h`

## TODO

### 1. ディスクリプタヒープ記述

```cpp
#pragma once

#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// ディスクリプタヒープタイプ
    enum class ERHIDescriptorHeapType : uint8
    {
        /// CBV/SRV/UAV
        CBV_SRV_UAV,

        /// サンプラー
        Sampler,

        /// レンダーターゲットビュー
        RTV,

        /// デプスステンシルビュー
        DSV,
    };

    /// ディスクリプタヒープフラグ
    enum class ERHIDescriptorHeapFlags : uint32
    {
        None = 0,

        /// シェーダーから参照可能（GPUヒープ）
        ShaderVisible = 1 << 0,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIDescriptorHeapFlags)

    /// ディスクリプタヒープ記述
    struct RHI_API RHIDescriptorHeapDesc
    {
        /// ヒープタイプ
        ERHIDescriptorHeapType type = ERHIDescriptorHeapType::CBV_SRV_UAV;

        /// ディスクリプタ数
        uint32 numDescriptors = 0;

        /// フラグ
        ERHIDescriptorHeapFlags flags = ERHIDescriptorHeapFlags::None;

        /// ノードマスク（マルチGPU用）。
        uint32 nodeMask = 0;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// CBV/SRV/UAVヒープ
        static RHIDescriptorHeapDesc CBV_SRV_UAV(uint32 count, bool shaderVisible = true) {
            RHIDescriptorHeapDesc desc;
            desc.type = ERHIDescriptorHeapType::CBV_SRV_UAV;
            desc.numDescriptors = count;
            desc.flags = shaderVisible ? ERHIDescriptorHeapFlags::ShaderVisible
                                       : ERHIDescriptorHeapFlags::None;
            return desc;
        }

        /// サンプラーヒープ
        static RHIDescriptorHeapDesc Sampler(uint32 count, bool shaderVisible = true) {
            RHIDescriptorHeapDesc desc;
            desc.type = ERHIDescriptorHeapType::Sampler;
            desc.numDescriptors = count;
            desc.flags = shaderVisible ? ERHIDescriptorHeapFlags::ShaderVisible
                                       : ERHIDescriptorHeapFlags::None;
            return desc;
        }

        /// RTVヒープ
        static RHIDescriptorHeapDesc RTV(uint32 count) {
            RHIDescriptorHeapDesc desc;
            desc.type = ERHIDescriptorHeapType::RTV;
            desc.numDescriptors = count;
            desc.flags = ERHIDescriptorHeapFlags::None;  // RTVはシェーダー非可視
            return desc;
        }

        /// DSVヒープ
        static RHIDescriptorHeapDesc DSV(uint32 count) {
            RHIDescriptorHeapDesc desc;
            desc.type = ERHIDescriptorHeapType::DSV;
            desc.numDescriptors = count;
            desc.flags = ERHIDescriptorHeapFlags::None;  // DSVはシェーダー非可視
            return desc;
        }
    };
}
```

- [ ] ERHIDescriptorHeapType 列挙型
- [ ] ERHIDescriptorHeapFlags 列挙型
- [ ] RHIDescriptorHeapDesc 構造体

### 2. IRHIDescriptorHeap インターフェース

```cpp
namespace NS::RHI
{
    /// ディスクリプタヒープ
    class RHI_API IRHIDescriptorHeap : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(DescriptorHeap)

        virtual ~IRHIDescriptorHeap() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// ヒープタイプ取得
        virtual ERHIDescriptorHeapType GetType() const = 0;

        /// ディスクリプタ数取得
        virtual uint32 GetNumDescriptors() const = 0;

        /// シェーダー可視か
        virtual bool IsShaderVisible() const = 0;

        /// ディスクリプタインクリメントサイズ取得
        virtual uint32 GetDescriptorIncrementSize() const = 0;

        //=====================================================================
        // ハンドル取得
        //=====================================================================

        /// CPUハンドル取得（先頭）。
        virtual RHICPUDescriptorHandle GetCPUDescriptorHandleForHeapStart() const = 0;

        /// GPUハンドル取得（先頭、シェーダー可視のみ）。
        virtual RHIGPUDescriptorHandle GetGPUDescriptorHandleForHeapStart() const = 0;

        /// 指定インデックスのCPUハンドル取得
        RHICPUDescriptorHandle GetCPUDescriptorHandle(uint32 index) const {
            RHICPUDescriptorHandle handle = GetCPUDescriptorHandleForHeapStart();
            handle.ptr += static_cast<size_t>(index) * GetDescriptorIncrementSize();
            return handle;
        }

        /// 指定インデックスのGPUハンドル取得
        RHIGPUDescriptorHandle GetGPUDescriptorHandle(uint32 index) const {
            RHIGPUDescriptorHandle handle = GetGPUDescriptorHandleForHeapStart();
            handle.ptr += static_cast<uint64>(index) * GetDescriptorIncrementSize();
            return handle;
        }
    };

    using RHIDescriptorHeapRef = TRefCountPtr<IRHIDescriptorHeap>;
}
```

- [ ] IRHIDescriptorHeap インターフェース
- [ ] ハンドル取得メソッド

### 3. ディスクリプタヒープ作成

```cpp
namespace NS::RHI
{
    /// ディスクリプタヒープ作成（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// ディスクリプタヒープ作成
        virtual IRHIDescriptorHeap* CreateDescriptorHeap(
            const RHIDescriptorHeapDesc& desc,
            const char* debugName = nullptr) = 0;

        /// ディスクリプタインクリメントサイズ取得
        virtual uint32 GetDescriptorIncrementSize(ERHIDescriptorHeapType type) const = 0;

        /// 最大ディスクリプタ数取得
        virtual uint32 GetMaxDescriptorCount(ERHIDescriptorHeapType type) const = 0;
    };
}
```

- [ ] CreateDescriptorHeap
- [ ] GetDescriptorIncrementSize

### 4. ディスクリプタヒープアロケーター

```cpp
namespace NS::RHI
{
    /// ディスクリプタアロケーション
    struct RHI_API RHIDescriptorAllocation
    {
        /// CPUハンドル
        RHICPUDescriptorHandle cpuHandle = {};

        /// GPUハンドル（シェーダー可視時のみ有効）。
        RHIGPUDescriptorHandle gpuHandle = {};

        /// ヒープのインデックス
        uint32 heapIndex = 0;

        /// 連続デスクリプタ数
        uint32 count = 0;

        /// 所属ヒープ
        IRHIDescriptorHeap* heap = nullptr;

        /// 有効か
        bool IsValid() const { return heap != nullptr && count > 0; }

        /// N番目のCPUハンドル取得
        RHICPUDescriptorHandle GetCPUHandle(uint32 offset = 0) const {
            RHICPUDescriptorHandle h = cpuHandle;
            h.ptr += static_cast<size_t>(offset) * heap->GetDescriptorIncrementSize();
            return h;
        }

        /// N番目のGPUハンドル取得
        RHIGPUDescriptorHandle GetGPUHandle(uint32 offset = 0) const {
            RHIGPUDescriptorHandle h = gpuHandle;
            h.ptr += static_cast<uint64>(offset) * heap->GetDescriptorIncrementSize();
            return h;
        }
    };

    /// ディスクリプタヒープアロケーター
    /// ヒープからデスクリプタを動的に割り当て
    /// @threadsafety スレッドセーフではない。外部同期が必要。
    class RHI_API RHIDescriptorHeapAllocator
    {
    public:
        RHIDescriptorHeapAllocator() = default;

        /// 初期化
        bool Initialize(IRHIDescriptorHeap* heap);

        /// シャットダウン
        void Shutdown();

        /// ディスクリプタ割り当て
        /// @return 有効な割り当て。枯渇時はIsValid()==falseを返す。
        ///         デバッグビルドではRHI_ASSERTでフェイルファスト。
        RHIDescriptorAllocation Allocate(uint32 count = 1);

        /// ディスクリプタ解放
        void Free(const RHIDescriptorAllocation& allocation);

        /// 利用可能ディスクリプタ数
        uint32 GetAvailableCount() const { return m_freeCount; }

        /// 総デスクリプタ数
        uint32 GetTotalCount() const { return m_heap ? m_heap->GetNumDescriptors() : 0; }

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// リセット（ディスクリプタを解放）。
        void Reset();

    private:
        IRHIDescriptorHeap* m_heap = nullptr;

        /// フリーリスト管理
        struct FreeRange {
            uint32 start;
            uint32 count;
        };
        FreeRange* m_freeRanges = nullptr;
        uint32 m_freeRangeCount = 0;
        uint32 m_freeRangeCapacity = 0;
        uint32 m_freeCount = 0;
    };
}
```

- [ ] RHIDescriptorAllocation 構造体
- [ ] RHIDescriptorHeapAllocator クラス

### 5. ディスクリプタコピー

```cpp
namespace NS::RHI
{
    /// ディスクリプタコピー（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// 単一ディスクリプタコピー
        virtual void CopyDescriptor(
            RHICPUDescriptorHandle destHandle,
            RHICPUDescriptorHandle srcHandle,
            ERHIDescriptorHeapType type) = 0;

        /// 複数ディスクリプタコピー
        virtual void CopyDescriptors(
            uint32 numDestDescriptorRanges,
            const RHICPUDescriptorHandle* destDescriptorRangeStarts,
            const uint32* destDescriptorRangeSizes,
            uint32 numSrcDescriptorRanges,
            const RHICPUDescriptorHandle* srcDescriptorRangeStarts,
            const uint32* srcDescriptorRangeSizes,
            ERHIDescriptorHeapType type) = 0;

        /// 簡易版（連続デスクリプタコピー
        void CopyDescriptorsSimple(
            RHICPUDescriptorHandle destStart,
            RHICPUDescriptorHandle srcStart,
            uint32 count,
            ERHIDescriptorHeapType type)
        {
            CopyDescriptors(1, &destStart, &count, 1, &srcStart, &count, type);
        }
    };
}
```

- [ ] CopyDescriptor
- [ ] CopyDescriptors

## 枯渇時フォールバック

1. デバッグビルド: RHI_ASSERTでフェイルファスト（サイズ見積もりミスの早期検出）
2. リリースビルド: ログ警告 + ヒープ自動拡張（現サイズ×2、上限あり）
3. 拡張不可時: nullptr返却、呼び出し元でフレームスキップ

## 検証方法

- [ ] ヒープ作成/破棄
- [ ] ディスクリプタ割り当て/解放
- [ ] ハンドル計算の正確性
- [ ] ディスクリプタコピー
