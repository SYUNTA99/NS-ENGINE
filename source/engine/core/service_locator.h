//----------------------------------------------------------------------------
//! @file   service_locator.h
//! @brief  サービスロケーターパターン実装
//----------------------------------------------------------------------------
#pragma once


#include "common/stl/stl_common.h"

// 前方宣言
class TextureManager;
class InputManager;
// CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行
class SpriteBatch;
class SceneManager;
class FileSystemManager;
class ShaderManager;
class IJobSystem;

//============================================================================
//! @brief サービスロケーター
//!
//! シングルトンへの直接依存を緩和し、テスト時のモック差し替えを可能にする。
//!
//! @note 使用例:
//! @code
//!   // 従来: TextureManager::Get().Load(...)
//!   // 新方式: Services::Textures().Load(...)
//!
//!   // テスト時:
//!   MockTextureManager mock;
//!   Services::Provide(&mock);
//! @endcode
//============================================================================
class Services final
{
public:
    //------------------------------------------------------------------------
    // サービス登録（エンジン初期化時に呼び出し）
    //------------------------------------------------------------------------

    static void Provide(TextureManager* tm) noexcept { textureManager_ = tm; }
    static void Provide(InputManager* im) noexcept { inputManager_ = im; }
    // CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行
    static void Provide(SpriteBatch* sb) noexcept { spriteBatch_ = sb; }
    static void Provide(SceneManager* sm) noexcept { sceneManager_ = sm; }
    static void Provide(FileSystemManager* fs) noexcept { fileSystem_ = fs; }
    static void Provide(ShaderManager* shader) noexcept { shaderManager_ = shader; }
    static void Provide(IJobSystem* js) noexcept { jobSystem_ = js; }

    //------------------------------------------------------------------------
    // サービス取得
    //------------------------------------------------------------------------

    [[nodiscard]] static TextureManager& Textures() noexcept {
        assert(textureManager_ && "TextureManager not provided");
        return *textureManager_;
    }

    [[nodiscard]] static InputManager& Input() noexcept {
        assert(inputManager_ && "InputManager not provided");
        return *inputManager_;
    }

    // CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行

    [[nodiscard]] static SpriteBatch& Sprites() noexcept {
        assert(spriteBatch_ && "SpriteBatch not provided");
        return *spriteBatch_;
    }

    [[nodiscard]] static SceneManager& Scenes() noexcept {
        assert(sceneManager_ && "SceneManager not provided");
        return *sceneManager_;
    }

    [[nodiscard]] static FileSystemManager& FileSystem() noexcept {
        assert(fileSystem_ && "FileSystemManager not provided");
        return *fileSystem_;
    }

    [[nodiscard]] static ShaderManager& Shaders() noexcept {
        assert(shaderManager_ && "ShaderManager not provided");
        return *shaderManager_;
    }

    [[nodiscard]] static IJobSystem& Jobs() noexcept {
        assert(jobSystem_ && "JobSystem not provided");
        return *jobSystem_;
    }

    //------------------------------------------------------------------------
    // 存在確認（オプショナル取得用）
    //------------------------------------------------------------------------

    [[nodiscard]] static bool HasTextures() noexcept { return textureManager_ != nullptr; }
    [[nodiscard]] static bool HasInput() noexcept { return inputManager_ != nullptr; }
    // CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行
    [[nodiscard]] static bool HasSprites() noexcept { return spriteBatch_ != nullptr; }
    [[nodiscard]] static bool HasScenes() noexcept { return sceneManager_ != nullptr; }
    [[nodiscard]] static bool HasFileSystem() noexcept { return fileSystem_ != nullptr; }
    [[nodiscard]] static bool HasShaders() noexcept { return shaderManager_ != nullptr; }
    [[nodiscard]] static bool HasJobs() noexcept { return jobSystem_ != nullptr; }

    //------------------------------------------------------------------------
    // クリーンアップ
    //------------------------------------------------------------------------

    static void Clear() noexcept {
        textureManager_ = nullptr;
        inputManager_ = nullptr;
        // CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行
        spriteBatch_ = nullptr;
        sceneManager_ = nullptr;
        fileSystem_ = nullptr;
        shaderManager_ = nullptr;
        jobSystem_ = nullptr;
    }

private:
    Services() = delete;  // インスタンス化禁止

    static inline TextureManager* textureManager_ = nullptr;
    static inline InputManager* inputManager_ = nullptr;
    // CollisionManager削除 - ECS::Collision2DSystem/Collision3DSystemに移行
    static inline SpriteBatch* spriteBatch_ = nullptr;
    static inline SceneManager* sceneManager_ = nullptr;
    static inline FileSystemManager* fileSystem_ = nullptr;
    static inline ShaderManager* shaderManager_ = nullptr;
    static inline IJobSystem* jobSystem_ = nullptr;
};
