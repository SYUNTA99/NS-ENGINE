//----------------------------------------------------------------------------
//! @file   collider2d_data.h
//! @brief  ECS Collider2DData - 2D衝突判定データ構造体
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include <cstdint>

namespace ECS {

//============================================================================
//! @brief 2D衝突判定データ（POD構造体）
//!
//! CollisionManager が実データを所有し、このデータは参照として機能する。
//! インデックス＋世代番号で削除・再利用を安全に検出。
//!
//! @note メモリレイアウト: 48 bytes, 16B境界アライン
//============================================================================
struct alignas(16) Collider2DData : public IComponentData {
    //------------------------------------------------------------------------
    // ホットデータ（毎フレーム衝突判定で使用）
    //------------------------------------------------------------------------
    float posX = 0.0f;              //!< ワールドX座標
    float posY = 0.0f;              //!< ワールドY座標
    float halfW = 0.0f;             //!< 半幅（AABB用）
    float halfH = 0.0f;             //!< 半高（AABB用）

    //------------------------------------------------------------------------
    // ウォームデータ（設定時・イベント時）
    //------------------------------------------------------------------------
    float offsetX = 0.0f;           //!< Transform中心からのオフセットX
    float offsetY = 0.0f;           //!< Transform中心からのオフセットY
    float sizeW = 0.0f;             //!< コライダー幅
    float sizeH = 0.0f;             //!< コライダー高さ

    //------------------------------------------------------------------------
    // フラグ・レイヤー
    //------------------------------------------------------------------------
    uint8_t layer = 0;              //!< 衝突レイヤー
    uint8_t mask = 0xFF;            //!< 衝突マスク
    uint8_t flags = 0x01;           //!< enabled(bit0), trigger(bit1)
    uint8_t _pad0 = 0;

    //------------------------------------------------------------------------
    // CollisionManagerハンドル
    //------------------------------------------------------------------------
    uint16_t colliderIndex = 0xFFFF;     //!< CollisionManager内インデックス
    uint16_t colliderGeneration = 0;     //!< 世代番号（再利用検出）

    //------------------------------------------------------------------------
    // ユーザーデータ
    //------------------------------------------------------------------------
    void* userData = nullptr;       //!< 任意のユーザーデータ

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief 有効かどうか
    [[nodiscard]] bool IsEnabled() const noexcept {
        return (flags & 0x01) != 0;
    }

    //! @brief トリガーモードかどうか
    [[nodiscard]] bool IsTrigger() const noexcept {
        return (flags & 0x02) != 0;
    }

    //! @brief 有効状態を設定
    void SetEnabled(bool enabled) noexcept {
        flags = enabled ? (flags | 0x01) : (flags & ~0x01);
    }

    //! @brief トリガーモードを設定
    void SetTrigger(bool trigger) noexcept {
        flags = trigger ? (flags | 0x02) : (flags & ~0x02);
    }

    //! @brief CollisionManagerに登録済みか
    [[nodiscard]] bool IsRegistered() const noexcept {
        return colliderIndex != 0xFFFF;
    }

    //! @brief サイズを設定（halfW/halfHも更新）
    void SetSize(float w, float h) noexcept {
        sizeW = w;
        sizeH = h;
        halfW = w * 0.5f;
        halfH = h * 0.5f;
    }

    //! @brief オフセットを設定
    void SetOffset(float x, float y) noexcept {
        offsetX = x;
        offsetY = y;
    }

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Collider2DData() = default;

    Collider2DData(float width, float height)
        : sizeW(width), sizeH(height)
        , halfW(width * 0.5f), halfH(height * 0.5f) {}

    Collider2DData(float width, float height, float offX, float offY)
        : offsetX(offX), offsetY(offY)
        , sizeW(width), sizeH(height)
        , halfW(width * 0.5f), halfH(height * 0.5f) {}
};

} // namespace ECS
