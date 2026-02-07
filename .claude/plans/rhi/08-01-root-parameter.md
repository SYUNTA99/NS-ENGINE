# 08-01: ルートパラメータ

## 目的

ルートシグネチャのルートパラメータ定義を定義する。

## 参照ドキュメント

- 01-06-enums-shader.md (EShaderVisibility)
- 01-07-enums-access.md (ERHIDescriptorType)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIRootSignature.h` (部分

## TODO

### 1. ルートパラメータタイプ

```cpp
#pragma once

#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// ルートパラメータタイプ
    enum class ERHIRootParameterType : uint8
    {
        /// ディスクリプタテーブル（複数ディスクリプタ）。
        DescriptorTable,

        /// ルート定数（2bit値、直接埋め込み）。
        Constants,

        /// ルートCBV（アドレス直接）。
        CBV,

        /// ルートSRV（アドレス直接）。
        SRV,

        /// ルートUAV（アドレス直接）。
        UAV,
    };

    /// シェーダービジビリティ
    enum class ERHIShaderVisibility : uint8
    {
        All,        // 全ステージ
        Vertex,
        Hull,
        Domain,
        Geometry,
        Pixel,
        Amplification,  // メッシュシェーダー用
        Mesh,           // メッシュシェーダー用
    };
}
```

- [ ] ERHIRootParameterType 列挙型
- [ ] ERHIShaderVisibility 列挙型

### 2. ルート定数パラメータ

```cpp
namespace NS::RHI
{
    /// ルート定数パラメータ記述
    struct RHI_API RHIRootConstants
    {
        /// バインドするシェーダーレジスタ（0, b1, ...）。
        uint32 shaderRegister = 0;

        /// レジスタスペース
        uint32 registerSpace = 0;

        /// 32bit定数の数
        uint32 num32BitValues = 0;

        /// ビルダー
        static RHIRootConstants Create(
            uint32 reg,
            uint32 numValues,
            uint32 space = 0)
        {
            RHIRootConstants rc;
            rc.shaderRegister = reg;
            rc.registerSpace = space;
            rc.num32BitValues = numValues;
            return rc;
        }

        /// テンプレート版（構造体サイズから自動計算）
        template<typename T>
        static RHIRootConstants CreateForType(uint32 reg, uint32 space = 0) {
            static_assert(sizeof(T) % 4 == 0, "Size must be multiple of 4 bytes");
            return Create(reg, sizeof(T) / 4, space);
        }
    };
}
```

- [ ] RHIRootConstants 構造体
- [ ] CreateForType テンプレート

### 3. ルートデスクリプタパラメータ

```cpp
namespace NS::RHI
{
    /// ルートデスクリプタパラメータ記述
    /// ルートCBV/SRV/UAV用
    struct RHI_API RHIRootDescriptor
    {
        /// バインドするシェーダーレジスタ
        uint32 shaderRegister = 0;

        /// レジスタスペース
        uint32 registerSpace = 0;

        /// フラグ
        enum class Flags : uint32 {
            None = 0,
            /// データが静的（コマンドリスト実行中に変更されない
            DataStatic = 1 << 0,
            /// データが揮発性（毎描画変更）。
            DataVolatile = 1 << 1,
            /// ディスクリプタが揮発性
            DescriptorsVolatile = 1 << 2,
        };
        Flags flags = Flags::None;

        /// ビルダー
        static RHIRootDescriptor CBV(uint32 reg, uint32 space = 0, Flags f = Flags::None) {
            RHIRootDescriptor rd;
            rd.shaderRegister = reg;
            rd.registerSpace = space;
            rd.flags = f;
            return rd;
        }

        static RHIRootDescriptor SRV(uint32 reg, uint32 space = 0, Flags f = Flags::None) {
            return CBV(reg, space, f);
        }

        static RHIRootDescriptor UAV(uint32 reg, uint32 space = 0, Flags f = Flags::None) {
            return CBV(reg, space, f);
        }
    };
    RHI_ENUM_CLASS_FLAGS(RHIRootDescriptor::Flags)
}
```

- [ ] RHIRootDescriptor 構造体
- [ ] CBV / SRV / UAV ビルダー

### 4. ルートパラメータ

```cpp
namespace NS::RHI
{
    /// ルートパラメータ記述
    struct RHI_API RHIRootParameter
    {
        /// パラメータタイプ
        ERHIRootParameterType parameterType = ERHIRootParameterType::DescriptorTable;

        /// シェーダービジビリティ
        ERHIShaderVisibility shaderVisibility = ERHIShaderVisibility::All;

        /// パラメータデータ
        union {
            /// ディスクリプタテーブル用
            struct {
                /// ディスクリプタレンジ配列へのポインタ
                const struct RHIDescriptorRange* ranges;
                /// レンジ数
                uint32 rangeCount;
            } descriptorTable;

            /// ルート定数用
            RHIRootConstants constants;

            /// ルートデスクリプタ用
            RHIRootDescriptor descriptor;
        };

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// ディスクリプタテーブルパラメータ
        static RHIRootParameter DescriptorTable(
            const RHIDescriptorRange* ranges,
            uint32 rangeCount,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::DescriptorTable;
            param.shaderVisibility = visibility;
            param.descriptorTable.ranges = ranges;
            param.descriptorTable.rangeCount = rangeCount;
            return param;
        }

        /// ルート定数パラメータ
        static RHIRootParameter Constants(
            uint32 shaderRegister,
            uint32 num32BitValues,
            uint32 registerSpace = 0,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::Constants;
            param.shaderVisibility = visibility;
            param.constants = RHIRootConstants::Create(shaderRegister, num32BitValues, registerSpace);
            return param;
        }

        /// ルートCBVパラメータ
        static RHIRootParameter CBV(
            uint32 shaderRegister,
            uint32 registerSpace = 0,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::CBV;
            param.shaderVisibility = visibility;
            param.descriptor = RHIRootDescriptor::CBV(shaderRegister, registerSpace);
            return param;
        }

        /// ルートSRVパラメータ
        static RHIRootParameter SRV(
            uint32 shaderRegister,
            uint32 registerSpace = 0,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::SRV;
            param.shaderVisibility = visibility;
            param.descriptor = RHIRootDescriptor::SRV(shaderRegister, registerSpace);
            return param;
        }

        /// ルートUAVパラメータ
        static RHIRootParameter UAV(
            uint32 shaderRegister,
            uint32 registerSpace = 0,
            ERHIShaderVisibility visibility = ERHIShaderVisibility::All)
        {
            RHIRootParameter param;
            param.parameterType = ERHIRootParameterType::UAV;
            param.shaderVisibility = visibility;
            param.descriptor = RHIRootDescriptor::UAV(shaderRegister, registerSpace);
            return param;
        }
    };
}
```

- [ ] RHIRootParameter 構造体
- [ ] ビルダーメソッド

### 5. ルートパラメータコスト計算

```cpp
namespace NS::RHI
{
    /// ルートシグネチャコスト制限
    constexpr uint32 kMaxRootSignatureCost = 64;  // DWORDs

    /// ルートパラメータコスト取得
    /// D3D12: ルートシグネチャは最大64 DWORD
    inline uint32 GetRootParameterCost(const RHIRootParameter& param)
    {
        switch (param.parameterType) {
            case ERHIRootParameterType::DescriptorTable:
                // ディスクリプタテーブル = 1 DWORD（GPUハンドル）。
                return 1;

            case ERHIRootParameterType::Constants:
                // 32bit定数 = N DWORDs
                return param.constants.num32BitValues;

            case ERHIRootParameterType::CBV:
            case ERHIRootParameterType::SRV:
            case ERHIRootParameterType::UAV:
                // ルートデスクリプタ = 2 DWORDs（4bitアドレス）。
                return 2;

            default:
                return 0;
        }
    }

    /// ルートパラメータ配列のコスト合計計算
    inline uint32 CalculateTotalRootParameterCost(
        const RHIRootParameter* params,
        uint32 paramCount)
    {
        uint32 totalCost = 0;
        for (uint32 i = 0; i < paramCount; ++i) {
            totalCost += GetRootParameterCost(params[i]);
        }
        return totalCost;
    }

    /// コスト制限のか検証
    inline bool ValidateRootSignatureCost(
        const RHIRootParameter* params,
        uint32 paramCount)
    {
        return CalculateTotalRootParameterCost(params, paramCount) <= kMaxRootSignatureCost;
    }
}
```

- [ ] kMaxRootSignatureCost 定数
- [ ] GetRootParameterCost
- [ ] CalculateTotalRootParameterCost
- [ ] ValidateRootSignatureCost

## 検証方法

- [ ] ルートパラメータの整合性
- [ ] コスト計算の正確性
- [ ] ビルダーの動作確認
