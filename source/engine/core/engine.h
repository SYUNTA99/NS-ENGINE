//----------------------------------------------------------------------------
//! @file   engine.h
//! @brief  エンジンコア - シングルトン管理とインフラ層
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/world.h"

// 前方宣言
class Engine;

//============================================================================
// グローバル変数
//============================================================================
extern Engine g_Engine;
extern ECS::World g_World;

//============================================================================
//! @brief エンジンクラス
//!
//! Application（プラットフォーム）とGame（ゲームロジック）の間のインフラ層。
//! - シングルトンマネージャーの作成/破棄
//! - FileSystemマウント
//! - ServiceLocator登録
//! - リソースクリーンアップ
//============================================================================
class Engine final
{
public:
    Engine() = default;
    ~Engine() = default;

    // コピー禁止
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    //------------------------------------------------------------------------
    //! @brief エンジン初期化
    //! @return 成功した場合true
    //------------------------------------------------------------------------
    [[nodiscard]] bool Initialize();

    //------------------------------------------------------------------------
    //! @brief エンジン終了
    //------------------------------------------------------------------------
    void Shutdown() noexcept;

    //------------------------------------------------------------------------
    //! @brief 初期化済みかどうか
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }

    //------------------------------------------------------------------------
    //! @brief World参照を取得
    //------------------------------------------------------------------------
    [[nodiscard]] ECS::World& GetWorld() noexcept { return g_World; }
    [[nodiscard]] const ECS::World& GetWorld() const noexcept { return g_World; }

private:
    //! @brief シングルトン作成
    bool CreateSingletons();

    //! @brief シングルトン破棄（逆順）
    void DestroySingletons() noexcept;

    //! @brief ファイルシステムマウント
    void MountFileSystems();

    //! @brief ServiceLocator登録
    void RegisterServices();

    //! @brief サブシステム初期化
    bool InitializeSubsystems();

    bool initialized_ = false;
};
