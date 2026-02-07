//----------------------------------------------------------------------------
//! @file   light_component_data.h
//! @brief  ECS LightComponentData - ライトデータ構造体
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"
#include "engine/lighting/light.h"

namespace ECS {

//============================================================================
//! @brief ライトコンポーネントデータ（POD構造体）
//!
//! LightDataをラップし、ECSコンポーネントとして使用可能にする。
//! LightingSystemがこれを収集してGPUバッファを更新する。
//!
//! @note 既存のLightData（64バイト）をそのまま内包
//============================================================================
struct alignas(16) LightComponentData : public IComponentData {
    //------------------------------------------------------------------------
    // コアデータ（GPU定数バッファと互換）
    //------------------------------------------------------------------------
    LightData gpuData = {};         //!< GPU用ライトデータ (64 bytes)

    //------------------------------------------------------------------------
    // ECS管理用
    //------------------------------------------------------------------------
    bool enabled = true;            //!< 有効フラグ
    bool castShadow = false;        //!< シャドウキャスト
    uint8_t _pad[2] = {};

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief ライトタイプを取得
    [[nodiscard]] LightType GetType() const noexcept {
        return static_cast<LightType>(static_cast<uint32_t>(gpuData.position.w));
    }

    //! @brief 位置を取得（PointLight/SpotLight用）
    [[nodiscard]] Vector3 GetPosition() const noexcept {
        return Vector3(gpuData.position.x, gpuData.position.y, gpuData.position.z);
    }

    //! @brief 位置を設定
    void SetPosition(const Vector3& pos) noexcept {
        gpuData.position.x = pos.x;
        gpuData.position.y = pos.y;
        gpuData.position.z = pos.z;
    }

    //! @brief 方向を取得
    [[nodiscard]] Vector3 GetDirection() const noexcept {
        return Vector3(gpuData.direction.x, gpuData.direction.y, gpuData.direction.z);
    }

    //! @brief 方向を設定
    void SetDirection(const Vector3& dir) noexcept {
        Vector3 normalized = dir;
        normalized.Normalize();
        gpuData.direction.x = normalized.x;
        gpuData.direction.y = normalized.y;
        gpuData.direction.z = normalized.z;
    }

    //! @brief 色を取得
    [[nodiscard]] Color GetColor() const noexcept {
        return Color(gpuData.color.R(), gpuData.color.G(), gpuData.color.B(), 1.0f);
    }

    //! @brief 色を設定
    void SetColor(const Color& color) noexcept {
        float intensity = gpuData.color.A();
        gpuData.color = Color(color.R(), color.G(), color.B(), intensity);
    }

    //! @brief 強度を取得
    [[nodiscard]] float GetIntensity() const noexcept {
        return gpuData.color.A();
    }

    //! @brief 強度を設定
    void SetIntensity(float intensity) noexcept {
        gpuData.color = Color(gpuData.color.R(), gpuData.color.G(), gpuData.color.B(), intensity);
    }

    //! @brief 範囲を取得（PointLight/SpotLight用）
    [[nodiscard]] float GetRange() const noexcept {
        return gpuData.direction.w;
    }

    //! @brief 範囲を設定
    void SetRange(float range) noexcept {
        gpuData.direction.w = range;
    }

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    LightComponentData() = default;

    //------------------------------------------------------------------------
    // ファクトリ関数
    //------------------------------------------------------------------------

    //! @brief 平行光源として初期化
    [[nodiscard]] static LightComponentData Directional(
        const Vector3& direction,
        const Color& color,
        float intensity) noexcept
    {
        LightComponentData data;
        data.gpuData = LightBuilder::Directional(direction, color, intensity);
        return data;
    }

    //! @brief 点光源として初期化
    [[nodiscard]] static LightComponentData Point(
        const Vector3& position,
        const Color& color,
        float intensity,
        float range) noexcept
    {
        LightComponentData data;
        data.gpuData = LightBuilder::Point(position, color, intensity, range);
        return data;
    }

    //! @brief スポットライトとして初期化
    [[nodiscard]] static LightComponentData Spot(
        const Vector3& position,
        const Vector3& direction,
        const Color& color,
        float intensity,
        float range,
        float innerAngleDeg,
        float outerAngleDeg) noexcept
    {
        LightComponentData data;
        data.gpuData = LightBuilder::Spot(position, direction, color, intensity,
                                          range, innerAngleDeg, outerAngleDeg);
        return data;
    }
};

ECS_COMPONENT(LightComponentData);

} // namespace ECS
