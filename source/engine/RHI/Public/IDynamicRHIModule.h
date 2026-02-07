/// @file IDynamicRHIModule.h
/// @brief RHIバックエンドモジュールインターフェース
/// @details バックエンド発見・選択・インスタンス化を担うモジュールインターフェース。
/// @see 01-15b-dynamic-rhi-module.md
#pragma once

#include "RHIFwd.h"
#include "RHIEnums.h"
#include "RHIMacros.h"

namespace NS::RHI
{
    //=========================================================================
    // IDynamicRHIModule
    //=========================================================================

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
        virtual bool IsSupported() const = 0;

        //=====================================================================
        // RHIインスタンス作成
        //=====================================================================

        /// IDynamicRHIインスタンスを作成
        /// IsSupported()がtrueの場合のみ呼び出すこと
        /// @return 作成されたRHIインスタンス（所有権は呼び出し側に移動）
        virtual IDynamicRHI* CreateRHI() = 0;
    };

    //=========================================================================
    // モジュール登録
    //=========================================================================

    /// モジュール登録ヘルパー
    struct RHI_API RHIModuleRegistrar
    {
        RHIModuleRegistrar(const char* name, IDynamicRHIModule* module);
    };

    /// 登録済みモジュール取得
    RHI_API IDynamicRHIModule* const* GetRegisteredRHIModules(uint32& count);

    /// 名前でモジュール検索
    RHI_API IDynamicRHIModule* FindRHIModule(const char* name);

    //=========================================================================
    // プラットフォームRHI作成
    //=========================================================================

    /// プラットフォームRHI作成
    /// バックエンドの優先順位に従いRHIを初期化する
    /// 結果は g_dynamicRHI に設定される
    RHI_API bool PlatformCreateDynamicRHI();

} // namespace NS::RHI
