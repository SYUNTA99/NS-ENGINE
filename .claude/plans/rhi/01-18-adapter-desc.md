# 01-18: アダプター記述情報

## 目的

GPUアダプターの情報を保持する構造体を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (3. Adapter実装詳細)
- docs/RHI/RHI_Implementation_Guide_Part9.md (ケイパビリティ)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIAdapterDesc.h`

## TODO

### 1. ベンダーID定数

```cpp
#pragma once

#include "common/utility/types.h"
#include "RHIEnums.h"
#include <string>

namespace NS::RHI
{
    //=========================================================================
    // ベンダーID定数
    //=========================================================================

    constexpr uint32 kVendorNVIDIA    = 0x10DE;
    constexpr uint32 kVendorAMD       = 0x1002;
    constexpr uint32 kVendorIntel     = 0x8086;
    constexpr uint32 kVendorQualcomm  = 0x5143;
    constexpr uint32 kVendorARM       = 0x13B5;
    constexpr uint32 kVendorImgTech   = 0x1010;
    constexpr uint32 kVendorMicrosoft = 0x1414;  // WARP
    constexpr uint32 kVendorApple     = 0x106B;

    /// ベンダー名取得
    inline const char* GetVendorName(uint32 vendorId)
    {
        switch (vendorId) {
            case kVendorNVIDIA:    return "NVIDIA";
            case kVendorAMD:       return "AMD";
            case kVendorIntel:     return "Intel";
            case kVendorQualcomm:  return "Qualcomm";
            case kVendorARM:       return "ARM";
            case kVendorImgTech:   return "Imagination Technologies";
            case kVendorMicrosoft: return "Microsoft";
            case kVendorApple:     return "Apple";
            default:               return "Unknown";
        }
    }
}
```

- [ ] ベンダーID定数
- [ ] GetVendorName関数

### 2. RHIAdapterDesc: 基本情報

```cpp
namespace NS::RHI
{
    /// アダプター記述情報
    struct RHI_API RHIAdapterDesc
    {
        //=====================================================================
        // 識別情報
        //=====================================================================

        /// システム内のアダプターインデックス
        uint32 adapterIndex = 0;

        /// GPU名（NVIDIA GeForce RTX 4090"等）
        std::string deviceName;

        /// ベンダーID
        uint32 vendorId = 0;

        /// デバイスID
        uint32 deviceId = 0;

        /// サブシステムID
        uint32 subsystemId = 0;

        /// リビジョン
        uint32 revision = 0;

        /// ドライババージョンのンダー固有フォーマット）
        uint64 driverVersion = 0;
    };
}
```

- [ ] 識別情報フィールド

### 3. RHIAdapterDesc: メモリ情報

```cpp
namespace NS::RHI
{
    struct RHIAdapterDesc
    {
        //=====================================================================
        // メモリ情報
        //=====================================================================

        /// 専用ビデオメモリ（RAM（バイト数
        uint64 dedicatedVideoMemory = 0;

        /// 専用システムメモリ（一部GPUで使用）。
        uint64 dedicatedSystemMemory = 0;

        /// 共有システムメモリ（統合GPU/APU）。
        uint64 sharedSystemMemory = 0;

        /// 統合メモリアーキテクチャか（APU等）
        bool unifiedMemory = false;
    };
}
```

- [ ] メモリ情報フィールド

### 4. RHIAdapterDesc: 機能レベル

```cpp
namespace NS::RHI
{
    struct RHIAdapterDesc
    {
        //=====================================================================
        // 機能レベル
        //=====================================================================

        /// 最大サポート機能レベル
        ERHIFeatureLevel maxFeatureLevel = ERHIFeatureLevel::SM5;

        /// 最大シェーダーモデル
        EShaderModel maxShaderModel = EShaderModel::SM5_0;

        /// リソースバインディングティア（D3D12用）。
        uint32 resourceBindingTier = 0;

        /// リソースヒープティア（D3D12用）。
        uint32 resourceHeapTier = 0;

        //=====================================================================
        // GPUノード
        //=====================================================================

        /// GPUノード数（SLI/CrossFire時の2以上）
        uint32 numDeviceNodes = 1;

        /// リンクされたアダプター（マルチGPU）か
        bool isLinkedAdapter = false;
    };
}
```

- [ ] 機能レベルフィールド
- [ ] GPUノード情報

### 5. RHIAdapterDesc: 機能フラグ

```cpp
namespace NS::RHI
{
    struct RHIAdapterDesc
    {
        //=====================================================================
        // 機能フラグ
        //=====================================================================

        /// レイトレーシングサポート
        bool supportsRayTracing = false;

        /// メッシュシェーダーサポート
        bool supportsMeshShaders = false;

        /// Bindlessサポート
        bool supportsBindless = false;

        /// 可変レートシェーディングサポート
        bool supportsVariableRateShading = false;

        /// Wave演算サポート
        bool supportsWaveOperations = false;

        /// 64ビットアトミックサポート
        bool supports64BitAtomics = false;

        /// タイルベースレンダリング（モバイルGPU）。
        bool isTileBased = false;

        /// ディスクリートGPUか（統合GPUでない）
        bool isDiscreteGPU = true;

        /// ソフトウェアレンダラーか（WARP等）
        bool isSoftwareAdapter = false;

        //=====================================================================
        // ユーティリティ
        //=====================================================================

        /// NVIDIAか
        bool IsNVIDIA() const { return vendorId == kVendorNVIDIA; }

        /// AMDか
        bool IsAMD() const { return vendorId == kVendorAMD; }

        /// Intelか
        bool IsIntel() const { return vendorId == kVendorIntel; }

        /// 統合GPUか
        bool IsIntegrated() const { return !isDiscreteGPU; }

        /// 合計RAMサイズ取得
        uint64 GetTotalVideoMemory() const {
            return dedicatedVideoMemory + sharedSystemMemory;
        }
    };
}
```

- [ ] 機能フラグ
- [ ] ユーティリティメソッド

## 検証方法

- [ ] フィールドの網羅性
- [ ] D3D12/Vulkan情報との対応
- [ ] 初期値の妥当性
