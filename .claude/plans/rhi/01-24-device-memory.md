# 01-24: IRHIDeviceメモリ管理

## 目的

IRHIDeviceのメモリアロケーション管理インターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (4. Device - メモリ管理
- docs/RHI/RHI_Implementation_Guide_Part3.md (メモリアロケーション)

## 変更対象ファイル

更新:
- `Source/Engine/RHI/Public/IRHIDevice.h`

## TODO

### 1. アロケーターインターフェース前方宣言

```cpp
namespace NS::RHI
{
    /// 前方宣言
    class IRHIBufferAllocator;
    class IRHITextureAllocator;
    class IRHIFastAllocator;
    class IRHIResidencyManager;
    class IRHIUploadHeap;
}
```

- [ ] 前方宣言

### 2. バッファアロケーター

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // バッファアロケーター
        //=====================================================================

        /// デフォルトバッファアロケーター取得
        /// 汎用バッファ用のプール管理アロケーター
        virtual IRHIBufferAllocator* GetDefaultBufferAllocator() = 0;

        /// 指定ヒープタイプのアロケーター取得
        /// @param heapType DEFAULT/UPLOAD/READBACK
        virtual IRHIBufferAllocator* GetBufferAllocator(
            ERHIHeapType heapType) = 0;
    };

    /// ヒープタイプ
    enum class ERHIHeapType : uint8
    {
        Default,    // GPU専用（VRAM、最高速）
        Upload,     // CPU書き込み→GPU読み取り
        Readback,   // GPU書き込み→CPU読み取り
    };
}
```

- [ ] GetDefaultBufferAllocator
- [ ] GetBufferAllocator
- [ ] ERHIHeapType

### 3. テクスチャアロケーター

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // テクスチャアロケーター
        //=====================================================================

        /// テクスチャアロケーター取得
        /// テクスチャ専用のメモリプール管理
        virtual IRHITextureAllocator* GetTextureAllocator() = 0;

        /// テクスチャメモリ要件取得
        /// @param desc テクスチャ記述情報
        /// @return 必要なメモリサイズとアライメント
        virtual RHIResourceAllocationInfo GetTextureAllocationInfo(
            const RHITextureDesc& desc) const = 0;

        /// バッファメモリ要件取得
        virtual RHIResourceAllocationInfo GetBufferAllocationInfo(
            const RHIBufferDesc& desc) const = 0;
    };

    /// リソース割り当て情報
    struct RHIResourceAllocationInfo
    {
        /// 必要なメモリサイズ（バイト）
        uint64 size = 0;

        /// アライメント要件（バイト）
        uint64 alignment = 0;
    };
}
```

- [ ] GetTextureAllocator
- [ ] GetTextureAllocationInfo / GetBufferAllocationInfo
- [ ] RHIResourceAllocationInfo

### 4. 高速アロケーター

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // 高速アロケーター
        //=====================================================================

        /// 高速アロケーター取得
        /// リングバッファベースの一時アロケーション用
        /// フレーム内の短寿命データに最適
        virtual IRHIFastAllocator* GetFastAllocator() = 0;

        /// アップロードヒープ取得
        /// CPU→GPU転送用のステージングメモリ
        virtual IRHIUploadHeap* GetUploadHeap() = 0;

        /// フレーム終了時のアロケーターリセット
        /// リングバッファの解放ポイント更新
        virtual void ResetFrameAllocators() = 0;
    };
}
```

- [ ] GetFastAllocator
- [ ] GetUploadHeap
- [ ] ResetFrameAllocators

### 5. Residency管理

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // Residency（メモリ常駐）管理
        //=====================================================================

        /// Residencyマネージャー取得
        /// VRAMオーバのコミット時のメモリ管理
        virtual IRHIResidencyManager* GetResidencyManager() = 0;

        /// リソースを常駐状態にする
        /// @param resources リソース配列
        /// @param count リソース数
        virtual void MakeResident(
            IRHIResource* const* resources,
            uint32 count) = 0;

        /// リソースを非常駐状態にする
        virtual void Evict(
            IRHIResource* const* resources,
            uint32 count) = 0;

        /// メモリ圧迫時のコールバック設定
        using MemoryPressureCallback = void(*)(uint64 bytesNeeded);
        virtual void SetMemoryPressureCallback(
            MemoryPressureCallback callback) = 0;

        //=====================================================================
        // メモリ統計
        //=====================================================================

        /// 現在のメモリ使用量取得
        virtual RHIMemoryStats GetMemoryStats() const = 0;
    };

    /// メモリ統計
    struct RHIMemoryStats
    {
        /// 割り当て済みメモリ（各ヒープタイプ）
        uint64 allocatedDefault = 0;
        uint64 allocatedUpload = 0;
        uint64 allocatedReadback = 0;

        /// 使用中メモリ
        uint64 usedDefault = 0;
        uint64 usedUpload = 0;
        uint64 usedReadback = 0;

        /// テクスチャメモリ
        uint64 textureMemory = 0;

        /// バッファメモリ
        uint64 bufferMemory = 0;

        /// 合計
        uint64 GetTotalAllocated() const {
            return allocatedDefault + allocatedUpload + allocatedReadback;
        }
        uint64 GetTotalUsed() const {
            return usedDefault + usedUpload + usedReadback;
        }
    };
}
```

- [ ] GetResidencyManager
- [ ] MakeResident / Evict
- [ ] メモリ統計

## 検証方法

- [ ] アロケーターの正常動作
- [ ] メモリ統計の正確性
- [ ] Residency管理の整合性
