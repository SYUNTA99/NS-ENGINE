//----------------------------------------------------------------------------
//! @file   lighting_system.h
//! @brief  ECS LightingSystem - ライティングシステム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/rendering/light_component_data.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/lighting/light.h"
#include "dx11/gpu/buffer.h"
#include <memory>

namespace ECS {

//============================================================================
//! @brief ライティングシステム（クエリシステム）
//!
//! 入力: LocalToWorldData, LightComponentData（読み取り専用）
//! 出力: GPU Buffer
//!
//! LightComponentDataを収集してGPU定数バッファを更新する。
//! 最大8ライトまで対応（kMaxLights制限）。
//!
//! 処理フロー:
//! 1. LocalToWorldData連携でワールド位置を更新
//! 2. 有効なLightComponentDataを収集
//! 3. LightingConstantsバッファを構築
//!
//! @note 優先度20（CollisionSystemの後）
//============================================================================
class LightingSystem final : public ISystem {
public:
    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------------------
    LightingSystem() = default;

    //------------------------------------------------------------------------
    //! @brief 初期化（定数バッファ作成）
    //------------------------------------------------------------------------
    void Initialize() {
        constantBuffer_ = std::make_unique<Buffer>();
        constantBuffer_->CreateConstant(sizeof(LightingConstants));
        initialized_ = true;
    }

    //------------------------------------------------------------------------
    //! @brief システム実行
    //------------------------------------------------------------------------
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        if (!initialized_) return;

        lightCount_ = 0;

        // 1. LocalToWorldData連携（PointLight/SpotLightの位置更新）
        world.ForEach<LocalToWorldData, LightComponentData>(
            [](Actor, const LocalToWorldData& ltw, LightComponentData& light) {
                if (!light.enabled) return;

                LightType type = light.GetType();
                if (type == LightType::Point || type == LightType::Spot) {
                    // LocalToWorldDataのワールド位置をライト位置に設定
                    Vector3 worldPos = ltw.GetPosition();
                    light.SetPosition(worldPos);
                }
            });

        // 2. LightComponentDataを収集
        world.ForEach<LightComponentData>([this](Actor, LightComponentData& light) {
            if (!light.enabled) return;
            if (lightCount_ >= kMaxLights) return;

            constants_.lights[lightCount_] = light.gpuData;
            ++lightCount_;
        });

        constants_.numLights = lightCount_;
        dirty_ = true;
    }

    int Priority() const override { return 20; }
    const char* Name() const override { return "LightingSystem"; }

    //------------------------------------------------------------------------
    //! @name 環境設定
    //------------------------------------------------------------------------
    //!@{

    //! @brief 環境光を設定
    void SetAmbientColor(const Color& color) noexcept {
        constants_.ambientColor = color;
        dirty_ = true;
    }

    //! @brief 環境光を取得
    [[nodiscard]] const Color& GetAmbientColor() const noexcept {
        return constants_.ambientColor;
    }

    //! @brief カメラ位置を設定（スペキュラ計算用）
    void SetCameraPosition(const Vector3& pos) noexcept {
        constants_.cameraPosition = Vector4(pos.x, pos.y, pos.z, 1.0f);
        dirty_ = true;
    }

    //!@}
    //------------------------------------------------------------------------
    //! @name GPU バインディング
    //------------------------------------------------------------------------
    //!@{

    //! @brief GPU定数バッファを更新・バインド
    //! @param slot 定数バッファスロット（通常b3）
    void Bind(uint32_t slot = 3) {
        if (!initialized_ || !constantBuffer_) return;

        if (dirty_) {
            constantBuffer_->Update(&constants_, sizeof(constants_));
            dirty_ = false;
        }

        constantBuffer_->BindVS(slot);
        constantBuffer_->BindPS(slot);
    }

    //!@}
    //------------------------------------------------------------------------
    //! @name 状態取得
    //------------------------------------------------------------------------
    //!@{

    //! @brief 有効ライト数を取得
    [[nodiscard]] uint32_t GetLightCount() const noexcept { return lightCount_; }

    //! @brief ライティング定数を取得
    [[nodiscard]] const LightingConstants& GetConstants() const noexcept {
        return constants_;
    }

    //! @brief 初期化済みかどうか
    [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }

    //!@}

private:
    LightingConstants constants_ = {};
    std::unique_ptr<Buffer> constantBuffer_;
    uint32_t lightCount_ = 0;
    bool dirty_ = true;
    bool initialized_ = false;
};

} // namespace ECS
