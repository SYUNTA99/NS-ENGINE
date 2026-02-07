# 01-19: IRHIAdapterインターフェース

## 目的

物理GPUを抽象化するIRHIAdapterインターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (3. Adapter実装詳細)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIAdapter.h`

## TODO

### 1. IRHIAdapter: 基本アクセス

```cpp
#pragma once

#include "RHIFwd.h"
#include "RHIAdapterDesc.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// GPUアダプターインターフェース
    /// 物理GPU（グラフィックスカード）を表す
    class RHI_API IRHIAdapter
    {
    public:
        virtual ~IRHIAdapter() = default;

        //=====================================================================
        // 基本アクセス
        //=====================================================================

        /// アダプター記述情報を取得
        virtual const RHIAdapterDesc& GetDesc() const = 0;

        /// アダプターインデックス取得
        uint32 GetAdapterIndex() const { return GetDesc().adapterIndex; }

        /// GPU名取得
        const char* GetDeviceName() const { return GetDesc().deviceName.c_str(); }

        /// ベンダーID取得
        uint32 GetVendorId() const { return GetDesc().vendorId; }
    };
}
```

- [ ] 基本アクセスメソッド
- [ ] Desc取得

### 2. IRHIAdapter: デバイス管理

```cpp
namespace NS::RHI
{
    class IRHIAdapter
    {
    public:
        //=====================================================================
        // デバイス管理
        //=====================================================================

        /// デバイスノード数取得
        /// @return 通常1、マルチGPU時の2以上
        virtual uint32 GetDeviceCount() const = 0;

        /// デバイス取得
        /// @param index デバイスインデックス（PUノード）
        /// @return デバイス（存在しない場合nullptr）。
        virtual IRHIDevice* GetDevice(uint32 index) const = 0;

        /// プライマリデバイス取得（インデックス0）。
        IRHIDevice* GetPrimaryDevice() const { return GetDevice(0); }
    };
}
```

- [ ] GetDeviceCount
- [ ] GetDevice

### 3. IRHIAdapter: 機能クエリ

```cpp
namespace NS::RHI
{
    class IRHIAdapter
    {
    public:
        //=====================================================================
        // 機能クエリ
        //=====================================================================

        /// 機能サポート確認
        /// @param feature 確認する機能
        /// @return サポート状態
        virtual ERHIFeatureSupport SupportsFeature(ERHIFeature feature) const = 0;

        /// 機能がサポートされているか（簡易版）。
        bool IsFeatureSupported(ERHIFeature feature) const {
            return SupportsFeature(feature) != ERHIFeatureSupport::Unsupported;
        }

        /// 最大機能レベル取得
        ERHIFeatureLevel GetMaxFeatureLevel() const {
            return GetDesc().maxFeatureLevel;
        }

        /// 指定機能レベルをサポートするか
        bool SupportsFeatureLevel(ERHIFeatureLevel level) const {
            return GetMaxFeatureLevel() >= level;
        }
    };
}
```

- [ ] SupportsFeature
- [ ] 機能レベルクエリ

### 4. IRHIAdapter: 制限値クエリ

```cpp
namespace NS::RHI
{
    class IRHIAdapter
    {
    public:
        //=====================================================================
        // 制限値クエリ
        //=====================================================================

        /// 最大テクスチャサイズ（3D/2Dの場合、各辺）。
        virtual uint32 GetMaxTextureSize() const = 0;

        /// 最大テクスチャ配列サイズ
        virtual uint32 GetMaxTextureArrayLayers() const = 0;

        /// 最大3Dテクスチャサイズ
        virtual uint32 GetMaxTexture3DSize() const = 0;

        /// 最大バッファサイズ
        virtual uint64 GetMaxBufferSize() const = 0;

        /// 最大コンスタントバッファサイズ
        virtual uint32 GetMaxConstantBufferSize() const = 0;

        /// 定数バッファアライメント
        virtual uint32 GetConstantBufferAlignment() const = 0;

        /// 構造化バッファストライドアライメント
        virtual uint32 GetStructuredBufferAlignment() const = 0;

        /// 最大サンプルカウント取得
        virtual ERHISampleCount GetMaxSampleCount(EPixelFormat format) const = 0;
    };
}
```

- [ ] テクスチャ制限
- [ ] バッファ制限
- [ ] アライメント要件

### 5. IRHIAdapter: メモリ・共有リソース

```cpp
namespace NS::RHI
{
    class IRHIAdapter
    {
    public:
        //=====================================================================
        // メモリ情報
        //=====================================================================

        /// 専用ビデオメモリ（RAM）取得
        uint64 GetDedicatedVideoMemory() const {
            return GetDesc().dedicatedVideoMemory;
        }

        /// 共有システムメモリ取得
        uint64 GetSharedSystemMemory() const {
            return GetDesc().sharedSystemMemory;
        }

        /// 統合メモリアーキテクチャか
        bool HasUnifiedMemory() const {
            return GetDesc().unifiedMemory;
        }

        //=====================================================================
        // 共有リソース管理（内部用）。
        //=====================================================================

        /// パイプラインステートキャッシュ取得
        virtual class IRHIPipelineStateCache* GetPipelineStateCache() = 0;

        /// ルートシグネチャマネージャー取得
        virtual class IRHIRootSignatureManager* GetRootSignatureManager() = 0;

        //=====================================================================
        // 出力（モニター）管理
        //=====================================================================

        /// 接続されてい出力数
        virtual uint32 GetOutputCount() const = 0;

        /// 出力がHDRをサポートするか
        virtual bool OutputSupportsHDR(uint32 outputIndex) const = 0;
    };
}
```

- [ ] メモリ情報を取得
- [ ] 共有リソースマネージャー
- [ ] 出力管理

## 検証方法

- [ ] インターフェース定義の完全性
- [ ] Descとの整合性
- [ ] 実装可能性確認
