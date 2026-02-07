# 08-04: バインディングレイアウト

## 目的

高レベルなバインディングレイアウト抽象化を定義する。

## 参照ドキュメント

- 08-03-root-signature.md (IRHIRootSignature)
- 06-03-shader-reflection.md (RHIBindingLayoutBuilder)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIBindingLayout.h`

## TODO

### 1. バインディングスロット定義

```cpp
#pragma once

#include "IRHIRootSignature.h"

namespace NS::RHI
{
    /// バインディングスロットタイプ
    enum class ERHIBindingSlotType : uint8
    {
        ConstantBuffer,     // CBV
        Texture,            // SRV (テクスチャ)
        Buffer,             // SRV (バッファ)
        RWTexture,          // UAV (テクスチャ)
        RWBuffer,           // UAV (バッファ)
        Sampler,            // サンプラー
    };

    /// バインディングスロット記述
    struct RHI_API RHIBindingSlot
    {
        /// スロット名（シェーダー変数名対応）
        const char* name = nullptr;

        /// スロットタイプ
        ERHIBindingSlotType type = ERHIBindingSlotType::ConstantBuffer;

        /// シェーダーステージマスク
        EShaderStageFlags stages = EShaderStageFlags::All;

        /// 配列サイズ（1 = 非配列）
        uint32 arraySize = 1;

        /// 更新頻度ヒント
        enum class UpdateFrequency : uint8 {
            PerFrame,       // フレームに1回
            PerMaterial,    // マテリアルごと
            PerObject,      // オブジェクトごと
            PerDraw,        // 描画ごと
        };
        UpdateFrequency updateFrequency = UpdateFrequency::PerDraw;

        //=====================================================================
        // ビルダー
        //=====================================================================

        static RHIBindingSlot CBV(const char* n, EShaderStageFlags s = EShaderStageFlags::All) {
            return {n, ERHIBindingSlotType::ConstantBuffer, s, 1, UpdateFrequency::PerDraw};
        }

        static RHIBindingSlot Texture(const char* n, EShaderStageFlags s = EShaderStageFlags::Pixel) {
            return {n, ERHIBindingSlotType::Texture, s, 1, UpdateFrequency::PerMaterial};
        }

        static RHIBindingSlot TextureArray(const char* n, uint32 count, EShaderStageFlags s = EShaderStageFlags::Pixel) {
            return {n, ERHIBindingSlotType::Texture, s, count, UpdateFrequency::PerMaterial};
        }

        static RHIBindingSlot Buffer(const char* n, EShaderStageFlags s = EShaderStageFlags::All) {
            return {n, ERHIBindingSlotType::Buffer, s, 1, UpdateFrequency::PerDraw};
        }

        static RHIBindingSlot RWTexture(const char* n, EShaderStageFlags s = EShaderStageFlags::Compute) {
            return {n, ERHIBindingSlotType::RWTexture, s, 1, UpdateFrequency::PerDraw};
        }

        static RHIBindingSlot RWBuffer(const char* n, EShaderStageFlags s = EShaderStageFlags::Compute) {
            return {n, ERHIBindingSlotType::RWBuffer, s, 1, UpdateFrequency::PerDraw};
        }

        static RHIBindingSlot Sampler(const char* n, EShaderStageFlags s = EShaderStageFlags::Pixel) {
            return {n, ERHIBindingSlotType::Sampler, s, 1, UpdateFrequency::PerMaterial};
        }
    };
}
```

- [ ] ERHIBindingSlotType 列挙型
- [ ] RHIBindingSlot 構造体

### 2. バインディングレイアウト記述

```cpp
namespace NS::RHI
{
    /// バインディングセット記述
    /// 更新頻度が同じバインディングをグループ化
    struct RHI_API RHIBindingSetDesc
    {
        /// セット名
        const char* name = nullptr;

        /// セットインデックス（ルートパラメータインデックスに対応）
        uint32 setIndex = 0;

        /// スロット配列
        const RHIBindingSlot* slots = nullptr;

        /// スロット数
        uint32 slotCount = 0;

        /// 更新頻度
        RHIBindingSlot::UpdateFrequency updateFrequency = RHIBindingSlot::UpdateFrequency::PerDraw;
    };

    /// バインディングレイアウト記述
    struct RHI_API RHIBindingLayoutDesc
    {
        /// バインディングセット配列
        const RHIBindingSetDesc* sets = nullptr;

        /// セット数
        uint32 setCount = 0;

        /// 静的サンプラー配列
        const RHIStaticSamplerDesc* staticSamplers = nullptr;

        /// 静的サンプラー数
        uint32 staticSamplerCount = 0;

        /// プッシュ定数サイズ（バイト）
        uint32 pushConstantSize = 0;

        /// プッシュ定数ステージ
        EShaderStageFlags pushConstantStages = EShaderStageFlags::All;
    };
}
```

- [ ] RHIBindingSetDesc 構造体
- [ ] RHIBindingLayoutDesc 構造体

### 3. バインディングレイアウトからルートシグネチャ変換

```cpp
namespace NS::RHI
{
    /// バインディングレイアウトのルートシグネチャ変換
    /// 変換規則:
    /// 1. 各RHIBindingSetDescは1つまたは2つのDescriptorTable RootParameterに変換
    ///    - CBV/SRV/UAVスロット → CBV_SRV_UAVヒープのDescriptorTable
    ///    - Samplerスロット → SamplerヒープのDescriptorTable（自動分離）
    /// 2. pushConstantSize > 0 の場合、RootConstants (register=0, space=999) を追加
    /// 3. UpdateFrequency::PerDraw のCBVはルートデスクリプタに昇格検討
    /// 4. StaticSamplerはDescriptorTableではなくStaticSamplerとして登録
    class RHI_API RHIBindingLayoutConverter
    {
    public:
        /// 変換
        /// @param layout バインディングレイアウト
        /// @param outDesc 出力ルートシグネチャ記述
        /// @return 成功したか
        static bool Convert(
            const RHIBindingLayoutDesc& layout,
            RHIRootSignatureDesc& outDesc);

        /// 変換（ビルダー出力版）。
        static bool Convert(
            const RHIBindingLayoutDesc& layout,
            RHIRootSignatureBuilder& outBuilder);

        /// スロットインデックスをルートパラメータインデックスに変換
        static bool GetRootParameterIndex(
            const RHIBindingLayoutDesc& layout,
            const char* slotName,
            uint32& outParamIndex,
            uint32& outTableOffset);
    };
}
```

- [ ] RHIBindingLayoutConverter クラス

### 4. 一般的なバインディングレイアウトのリセット

```cpp
namespace NS::RHI
{
    /// バインディングレイアウトのリセット
    namespace RHIBindingLayoutPresets
    {
        /// 最小限（CBV + 1テクスチャ + 1サンプラー）。
        inline RHIBindingLayoutDesc Minimal()
        {
            static RHIBindingSlot slots[] = {
                RHIBindingSlot::CBV("cbPerObject"),
                RHIBindingSlot::Texture("texAlbedo"),
            };
            static RHIStaticSamplerDesc samplers[] = {
                RHIStaticSamplerDesc::LinearWrap(0),
            };
            static RHIBindingSetDesc sets[] = {
                {"PerDraw", 0, slots, 2, RHIBindingSlot::UpdateFrequency::PerDraw},
            };
            return RHIBindingLayoutDesc{sets, 1, samplers, 1, 0, EShaderStageFlags::None};
        }

        /// PBR基本（CBV + 5テクスチャ）。
        inline RHIBindingLayoutDesc PBRBasic()
        {
            static RHIBindingSlot perFrameSlots[] = {
                RHIBindingSlot::CBV("cbPerFrame", EShaderStageFlags::All),
            };
            static RHIBindingSlot perMaterialSlots[] = {
                RHIBindingSlot::CBV("cbPerMaterial", EShaderStageFlags::Pixel),
                RHIBindingSlot::Texture("texAlbedo"),
                RHIBindingSlot::Texture("texNormal"),
                RHIBindingSlot::Texture("texMetalRoughness"),
                RHIBindingSlot::Texture("texAO"),
                RHIBindingSlot::Texture("texEmissive"),
            };
            static RHIBindingSlot perObjectSlots[] = {
                RHIBindingSlot::CBV("cbPerObject", EShaderStageFlags::Vertex),
            };
            static RHIStaticSamplerDesc samplers[] = {
                RHIStaticSamplerDesc::Anisotropic(0),
                RHIStaticSamplerDesc::LinearClamp(1),
            };
            static RHIBindingSetDesc sets[] = {
                {"PerFrame", 0, perFrameSlots, 1, RHIBindingSlot::UpdateFrequency::PerFrame},
                {"PerMaterial", 1, perMaterialSlots, 6, RHIBindingSlot::UpdateFrequency::PerMaterial},
                {"PerObject", 2, perObjectSlots, 1, RHIBindingSlot::UpdateFrequency::PerObject},
            };
            return RHIBindingLayoutDesc{sets, 3, samplers, 2, 0, EShaderStageFlags::None};
        }

        /// コンピュート基本
        inline RHIBindingLayoutDesc ComputeBasic()
        {
            static RHIBindingSlot slots[] = {
                RHIBindingSlot::CBV("cbParams", EShaderStageFlags::Compute),
                RHIBindingSlot::Buffer("bufInput", EShaderStageFlags::Compute),
                RHIBindingSlot::RWBuffer("bufOutput", EShaderStageFlags::Compute),
            };
            static RHIBindingSetDesc sets[] = {
                {"ComputeSet", 0, slots, 3, RHIBindingSlot::UpdateFrequency::PerDraw},
            };
            return RHIBindingLayoutDesc{sets, 1, nullptr, 0, 0, EShaderStageFlags::None};
        }

        /// ポストプロセス
        inline RHIBindingLayoutDesc PostProcess()
        {
            static RHIBindingSlot slots[] = {
                RHIBindingSlot::Texture("texInput", EShaderStageFlags::Pixel),
                RHIBindingSlot::RWTexture("texOutput", EShaderStageFlags::Pixel),
            };
            static RHIStaticSamplerDesc samplers[] = {
                RHIStaticSamplerDesc::PointClamp(0),
                RHIStaticSamplerDesc::LinearClamp(1),
            };
            static RHIBindingSetDesc sets[] = {
                {"PostProcess", 0, slots, 2, RHIBindingSlot::UpdateFrequency::PerDraw},
            };
            RHIBindingLayoutDesc desc;
            desc.sets = sets;
            desc.setCount = 1;
            desc.staticSamplers = samplers;
            desc.staticSamplerCount = 2;
            desc.pushConstantSize = 64;  // 4x vec4
            desc.pushConstantStages = EShaderStageFlags::Pixel;
            return desc;
        }
    }
}
```

- [ ] RHIBindingLayoutPresets

### 5. 実行時バインディングヘルパー

```cpp
namespace NS::RHI
{
    /// バインディングセットインスタンス
    /// 実際のリソースをバインディングレイアウトに従って設定
    class RHI_API RHIBindingSet
    {
    public:
        RHIBindingSet() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            const RHIBindingSetDesc& desc);

        /// CBV設定
        void SetCBV(const char* slotName, IRHIBuffer* buffer);
        void SetCBV(uint32 slotIndex, IRHIBuffer* buffer);

        /// SRV設定（テクスチャ）。
        void SetTexture(const char* slotName, IRHITexture* texture);
        void SetTexture(uint32 slotIndex, IRHITexture* texture);

        /// SRV設定（バッファ）。
        void SetBuffer(const char* slotName, IRHIBuffer* buffer);
        void SetBuffer(uint32 slotIndex, IRHIBuffer* buffer);

        /// UAV設定
        void SetRWTexture(const char* slotName, IRHITexture* texture);
        void SetRWBuffer(const char* slotName, IRHIBuffer* buffer);

        /// サンプラー設定
        void SetSampler(const char* slotName, IRHISampler* sampler);

        /// コンテキストにバインド
        void Bind(IRHICommandContext* context, uint32 rootParameterIndex);

        /// 有効か
        bool IsValid() const;

    private:
        IRHIDevice* m_device = nullptr;
        const RHIBindingSetDesc* m_desc = nullptr;
        // 内のディスクリプタテーブル管理
    };
}
```

- [ ] RHIBindingSet クラス

## 検証方法

- [ ] バインディングレイアウトの整合性
- [ ] ルートシグネチャへの変換
- [ ] プリセットの動作確認
- [ ] 実行時バインディングの正確性
