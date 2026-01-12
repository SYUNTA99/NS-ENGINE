//----------------------------------------------------------------------------
//! @file   game.h
//! @brief  ゲームクラス
//----------------------------------------------------------------------------
#pragma once

#include "engine/scene/scene.h"
#include <memory>

// 前方宣言
class SceneManager;

//----------------------------------------------------------------------------
//! @brief ゲームクラス
//!
//! シーンを所有し、ゲームループを実行する。
//! シーン切り替えはフレーム末に実行される。
//----------------------------------------------------------------------------
class Game final
{
public:
    Game();
    ~Game() noexcept = default;

    // コピー・ムーブ禁止
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    Game(Game&&) = delete;
    Game& operator=(Game&&) = delete;

    //----------------------------------------------------------
    //! @name ライフサイクル
    //----------------------------------------------------------
    //!@{

    //! @brief 初期化
    [[nodiscard]] bool Initialize();

    //! @brief 終了処理
    void Shutdown() noexcept;

    //!@}

    //----------------------------------------------------------
    //! @name フレームコールバック
    //----------------------------------------------------------
    //!@{

    //! @brief 固定タイムステップ更新（物理・ロジック）
    //! @param dt 固定デルタタイム（通常1/60秒）
    void FixedUpdate(float dt);

    //! @brief 更新（従来互換・可変タイムステップ時に呼ばれる）
    void Update();

    //! @brief 描画
    void Render();

    //! @brief フレーム末処理
    void EndFrame();

    //!@}

private:
    std::unique_ptr<Scene> currentScene_;     //!< 現在のシーン（所有）
};
