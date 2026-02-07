# 01-15b: IDynamicRHIModule — バックエンド発見・選択・インスタンス化

## 目的

RHIバックエンド（D3D12, Vulkan等）の発見・サポートチェック・インスタンス化を担うモジュールインターフェースを定義する。01-15のIDynamicRHIが「RHIの機能」を抽象化するのに対し、本計画はその「生成方法」を抽象化する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (FDynamicRHI実装詳細)

## 前提条件

- 01-15-dynamic-rhi-core（IDynamicRHI基本インターフェース）

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IDynamicRHIModule.h`

更新:
- `Source/Engine/RHI/Public/IDynamicRHI.h`（PlatformCreateDynamicRHI宣言追加）

## TODO

### 1. IDynamicRHIModule インターフェース

各バックエンド（D3D12RHIModule, VulkanRHIModule等）が実装するモジュールインターフェース。

```cpp
#pragma once

#include "RHIFwd.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// RHIバックエンドモジュールインターフェース
    /// 各バックエンド（D3D12, Vulkan等）がこれを実装する
    class RHI_API IDynamicRHIModule
    {
    public:
        virtual ~IDynamicRHIModule() = default;

        //=====================================================================
        // モジュール識別
        //=====================================================================

        /// モジュール名取得（"D3D12", "Vulkan"等）
        virtual const char* GetModuleName() const = 0;

        /// 対応するバックエンド種別
        virtual ERHIInterfaceType GetInterfaceType() const = 0;

        //=====================================================================
        // サポートチェック
        //=====================================================================

        /// このバックエンドが現在の環境でサポートされているか
        /// ドライバ存在チェック、APIバージョンチェック等を行う
        /// @return サポートされている場合true
        virtual bool IsSupported() const = 0;

        //=====================================================================
        // RHIインスタンス作成
        //=====================================================================

        /// IDynamicRHIインスタンスを作成
        /// IsSupported()がtrueの場合のみ呼び出すこと
        /// @return 作成されたRHIインスタンス（所有権は呼び出し側に移動）
        virtual IDynamicRHI* CreateRHI() = 0;
    };
}
```

- [ ] IDynamicRHIModule基本定義
- [ ] GetModuleName / GetInterfaceType
- [ ] IsSupported（ドライバ/APIバージョンチェック）
- [ ] CreateRHI

### 2. IsSupportedの実装ガイドライン

各バックエンドが`IsSupported()`で行うべきチェック:

**D3D12バックエンド:**
```
1. d3d12.dll のロード可否
2. D3D12CreateDevice() でFeature Level 12_0以上のデバイス作成可否
3. DXGI 1.4以上のファクトリ作成可否
```

**Vulkanバックエンド:**
```
1. vulkan-1.dll のロード可否
2. vkEnumerateInstanceVersion() で Vulkan 1.1以上
3. vkCreateInstance() 成功
4. 適切な物理デバイスの存在
```

- [ ] D3D12チェック項目の文書化
- [ ] Vulkanチェック項目の文書化

### 3. PlatformCreateDynamicRHI — エントリポイント

エンジン起動時にRHIを初期化するエントリポイント関数。

```cpp
namespace NS::RHI
{
    /// プラットフォームRHI作成
    /// バックエンドの優先順位に従いRHIを初期化する
    /// 結果は g_dynamicRHI に設定される
    ///
    /// @return 初期化成功時true
    ///
    /// バックエンド選択優先順位:
    ///   1. コマンドライン引数による明示指定（-d3d12, -vulkan）
    ///   2. D3D12（Windows環境のデフォルト）
    ///   3. Vulkan（フォールバック）
    bool PlatformCreateDynamicRHI();
}
```

**実装フロー:**

```
PlatformCreateDynamicRHI()
│
├─ コマンドライン引数チェック
│   ├─ "-d3d12" → D3D12のみ試行
│   └─ "-vulkan" → Vulkanのみ試行
│
├─ 明示指定なし: 優先順位順に試行
│   ├─ D3D12RHIModule::IsSupported() → true → CreateRHI()
│   └─ VulkanRHIModule::IsSupported() → true → CreateRHI()
│
├─ 作成成功
│   ├─ g_dynamicRHI = 作成されたインスタンス
│   ├─ g_dynamicRHI->Init()
│   ├─ g_dynamicRHI->PostInit()
│   └─ return true
│
└─ 全バックエンド失敗
    ├─ エラーログ出力
    └─ return false
```

- [ ] PlatformCreateDynamicRHI 宣言
- [ ] コマンドライン引数によるバックエンド選択
- [ ] 優先順位ベースのフォールバック
- [ ] g_dynamicRHI への設定
- [ ] エラーハンドリング

### 4. モジュール登録

バックエンドモジュールの静的登録メカニズム。

```cpp
namespace NS::RHI
{
    /// 登録済みRHIモジュール配列（静的初期化）
    /// 各バックエンドが自身のモジュールを登録する
    ///
    /// 使用例（D3D12バックエンド側）:
    /// ```cpp
    /// static D3D12RHIModule g_d3d12Module;
    /// static RHIModuleRegistrar g_d3d12Registrar("D3D12", &g_d3d12Module);
    /// ```

    /// モジュール登録ヘルパー
    struct RHI_API RHIModuleRegistrar
    {
        RHIModuleRegistrar(const char* name, IDynamicRHIModule* module);
    };

    /// 登録済みモジュール取得
    /// @param count 出力: モジュール数
    /// @return モジュール配列
    IDynamicRHIModule* const* GetRegisteredRHIModules(uint32& count);

    /// 名前でモジュール検索
    /// @param name モジュール名（"D3D12", "Vulkan"等）
    /// @return 見つかったモジュール（なければnullptr）
    IDynamicRHIModule* FindRHIModule(const char* name);
}
```

- [ ] RHIModuleRegistrar 登録ヘルパー
- [ ] GetRegisteredRHIModules
- [ ] FindRHIModule

## 既存計画との関係

| 計画 | 関係 |
|------|------|
| 01-15 (IDynamicRHI基本) | 本計画が生成するインスタンスの型 |
| 01-16 (リソースファクトリ) | IDynamicRHI上のファクトリメソッド（本計画の後に利用可能） |
| 01-05 (enums-core) | ERHIInterfaceType を使用 |

## 検証方法

- [ ] IDynamicRHIModuleインターフェースの完全性
- [ ] PlatformCreateDynamicRHIのフロー整合性
- [ ] モジュール登録メカニズムの妥当性
- [ ] 01-15のg_dynamicRHI/GetDynamicRHI()との整合性
