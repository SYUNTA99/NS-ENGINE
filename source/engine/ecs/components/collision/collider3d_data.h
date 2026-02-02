//----------------------------------------------------------------------------
//! @file   collider3d_data.h
//! @brief  ECS Collider3DData - 3D衝突判定データ構造体
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"
#include <cstdint>

namespace ECS {

//============================================================================
//! @brief 3Dコライダー形状タイプ
//============================================================================
enum class Collider3DShape : uint8_t {
    AABB = 0,      //!< 軸平行境界ボックス
    Sphere = 1,    //!< 球
    Capsule = 2    //!< カプセル
};

//============================================================================
//! @brief 3D衝突判定データ（POD構造体）
//!
//! @note メモリレイアウト: 96 bytes, 16B境界アライン
//!       - ホットデータ（AABB bounds）を先頭に配置
//!       - 形状固有データはunionで共有（メモリ効率）
//============================================================================
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
struct alignas(16) Collider3DData : public IComponentData {
    //------------------------------------------------------------------------
    // ホットデータ（毎フレーム衝突判定で使用）- 32 bytes
    //------------------------------------------------------------------------
    float minX = 0.0f, minY = 0.0f, minZ = 0.0f;  //!< AABB最小点 (12 bytes)
    float _pad0 = 0.0f;                            //!< アライメント (4 bytes)
    float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;  //!< AABB最大点 (12 bytes)
    float _pad1 = 0.0f;                            //!< アライメント (4 bytes)

    //------------------------------------------------------------------------
    // 形状固有データ（union - 16 bytes）
    //------------------------------------------------------------------------
    union ShapeData {
        struct { float halfExtentX, halfExtentY, halfExtentZ, _pad; } aabb;
        struct { float radius, _pad[3]; } sphere;
        struct { float radius, halfHeight, _pad[2]; } capsule;

        ShapeData() : aabb{0.5f, 0.5f, 0.5f, 0.0f} {}
    } shape;

    //------------------------------------------------------------------------
    // オフセット・中心（16 bytes）
    //------------------------------------------------------------------------
    float offsetX = 0.0f;              //!< Transform中心からのオフセットX
    float offsetY = 0.0f;              //!< Transform中心からのオフセットY
    float offsetZ = 0.0f;              //!< Transform中心からのオフセットZ
    float _pad2 = 0.0f;

    //------------------------------------------------------------------------
    // フラグ・レイヤー（8 bytes）
    //------------------------------------------------------------------------
    uint32_t layer = 0x01;             //!< 衝突レイヤー（32ビット）
    uint32_t mask = 0xFFFFFFFF;        //!< 衝突マスク（32ビット）

    //------------------------------------------------------------------------
    // 状態（8 bytes）
    //------------------------------------------------------------------------
    Collider3DShape shapeType = Collider3DShape::AABB;  //!< 形状タイプ
    uint8_t flags = 0x01;              //!< enabled(bit0), trigger(bit1), static(bit2)
    uint16_t _pad3 = 0;
    uint32_t _pad4 = 0;

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsEnabled() const noexcept { return (flags & 0x01) != 0; }
    [[nodiscard]] bool IsTrigger() const noexcept { return (flags & 0x02) != 0; }
    [[nodiscard]] bool IsStatic() const noexcept  { return (flags & 0x04) != 0; }

    void SetEnabled(bool v) noexcept { flags = v ? (flags | 0x01) : (flags & ~0x01); }
    void SetTrigger(bool v) noexcept { flags = v ? (flags | 0x02) : (flags & ~0x02); }
    void SetStatic(bool v) noexcept  { flags = v ? (flags | 0x04) : (flags & ~0x04); }

    //! @brief AABBとして初期化
    void SetAsAABB(float hx, float hy, float hz) noexcept {
        shapeType = Collider3DShape::AABB;
        shape.aabb = {hx, hy, hz, 0.0f};
    }

    //! @brief 球として初期化
    void SetAsSphere(float radius) noexcept {
        shapeType = Collider3DShape::Sphere;
        shape.sphere = {radius, {0.0f, 0.0f, 0.0f}};
    }

    //! @brief カプセルとして初期化
    void SetAsCapsule(float radius, float halfHeight) noexcept {
        shapeType = Collider3DShape::Capsule;
        shape.capsule = {radius, halfHeight, {0.0f, 0.0f}};
    }

    //! @brief ワールド位置からAABB境界を更新
    void UpdateBounds(const Vector3& worldPos) noexcept {
        Vector3 center = worldPos + Vector3(offsetX, offsetY, offsetZ);
        float extX, extY, extZ;

        switch (shapeType) {
        case Collider3DShape::AABB:
            extX = shape.aabb.halfExtentX;
            extY = shape.aabb.halfExtentY;
            extZ = shape.aabb.halfExtentZ;
            break;
        case Collider3DShape::Sphere:
            extX = extY = extZ = shape.sphere.radius;
            break;
        case Collider3DShape::Capsule:
            extX = extZ = shape.capsule.radius;
            extY = shape.capsule.halfHeight + shape.capsule.radius;
            break;
        default:
            extX = extY = extZ = 0.5f;
            break;
        }

        minX = center.x - extX;  minY = center.y - extY;  minZ = center.z - extZ;
        maxX = center.x + extX;  maxY = center.y + extY;  maxZ = center.z + extZ;
    }

    //! @brief AABB中心を取得
    [[nodiscard]] Vector3 GetCenter() const noexcept {
        return Vector3(
            (minX + maxX) * 0.5f,
            (minY + maxY) * 0.5f,
            (minZ + maxZ) * 0.5f
        );
    }

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Collider3DData() = default;

    //! @brief 球として初期化
    explicit Collider3DData(float radius)
        : shapeType(Collider3DShape::Sphere) {
        shape.sphere = {radius, {0.0f, 0.0f, 0.0f}};
    }

    //! @brief AABBとして初期化
    Collider3DData(float hx, float hy, float hz)
        : shapeType(Collider3DShape::AABB) {
        shape.aabb = {hx, hy, hz, 0.0f};
    }

    //! @brief カプセルとして初期化
    Collider3DData(float radius, float halfHeight, bool /*capsuleTag*/)
        : shapeType(Collider3DShape::Capsule) {
        shape.capsule = {radius, halfHeight, {0.0f, 0.0f}};
    }
};
#pragma warning(pop)

ECS_COMPONENT(Collider3DData);

} // namespace ECS
