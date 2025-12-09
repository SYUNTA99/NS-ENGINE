//----------------------------------------------------------------------------
//! @file   scene_manager.h
//! @brief  シーンマネージャー
//----------------------------------------------------------------------------
#pragma once

#include "scene.h"
#include <memory>

//----------------------------------------------------------------------------
//! @brief シーンマネージャー（シングルトン）
//!
//! シーン切り替えの予約を管理する。
//! シーンの所有はGame側で行う。
//----------------------------------------------------------------------------
class SceneManager final
{
public:
    //! @brief シングルトンインスタンス取得
    static SceneManager& Get() noexcept;

private:
    SceneManager() = default;
    ~SceneManager() = default;

    // コピー・ムーブ禁止
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager(SceneManager&&) = delete;
    SceneManager& operator=(SceneManager&&) = delete;

public:

    //----------------------------------------------------------
    //! @name シーン切り替え
    //----------------------------------------------------------
    //!@{

    //! @brief シーン読み込みを予約
    //! @tparam T シーンクラス（Sceneの派生クラス）
    template<typename T>
    void Load()
    {
        pendingFactory_ = &CreateScene<T>;
    }

    //! @brief 予約されたシーン切り替えを適用
    //! @param current 現在のシーン（参照で受け取り、切り替え後は新シーンになる）
    void ApplyPendingChange(std::unique_ptr<Scene>& current);

    //!@}

private:
    //! シーン生成ヘルパー
    template<typename T>
    static std::unique_ptr<Scene> CreateScene()
    {
        return std::make_unique<T>();
    }

    //! シーン生成関数の型
    using SceneFactory = std::unique_ptr<Scene>(*)();

    //! 次のシーン生成関数（切り替え予約）
    SceneFactory pendingFactory_ = nullptr;
};
