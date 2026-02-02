//----------------------------------------------------------------------------
//! @file   collider.h
//! @brief  Collider - 純粋OOP当たり判定コンポーネント
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component.h"
#include "engine/math/math_types.h"
#include "transform.h"
#include <functional>
#include <cstdint>

//============================================================================
// 前方宣言
//============================================================================
class Collider;
class BoxCollider;
class SphereCollider;
class CircleCollider;
class BoxCollider2D;

//============================================================================
//! @brief 衝突情報
//============================================================================
struct CollisionInfo {
    Collider* other = nullptr;          //!< 衝突相手
    Vector3 contactPoint;               //!< 接触点
    Vector3 normal;                     //!< 衝突法線
    float penetration = 0.0f;           //!< めり込み量
};

//============================================================================
//! @brief Collider基底クラス - 当たり判定の基底
//============================================================================
class Collider : public Component {
public:
    virtual ~Collider() = default;

    //------------------------------------------------------------------------
    // ライフサイクル
    //------------------------------------------------------------------------
    void Start() override {
        transform_ = GetComponent<Transform>();
    }

    //------------------------------------------------------------------------
    // 衝突判定
    //------------------------------------------------------------------------

    //! @brief 衝突判定（派生クラスで実装）
    [[nodiscard]] virtual bool Intersects(const Collider* other) const = 0;

    //! @brief 詳細な衝突情報を取得
    [[nodiscard]] virtual bool GetCollisionInfo(const Collider* other, CollisionInfo& info) const {
        info.other = const_cast<Collider*>(other);
        return Intersects(other);
    }

    //! @brief 点が内部にあるか
    [[nodiscard]] virtual bool ContainsPoint(const Vector3& point) const = 0;

    //! @brief レイキャスト
    [[nodiscard]] virtual bool Raycast(const Vector3& origin, const Vector3& direction,
                                        float maxDistance, float& hitDistance) const = 0;

    //------------------------------------------------------------------------
    // バウンディングボックス
    //------------------------------------------------------------------------

    //! @brief ワールド空間でのAABB取得
    [[nodiscard]] virtual void GetWorldBounds(Vector3& min, Vector3& max) const = 0;

    //------------------------------------------------------------------------
    // プロパティ
    //------------------------------------------------------------------------

    [[nodiscard]] bool IsTrigger() const noexcept { return isTrigger_; }
    [[nodiscard]] uint32_t GetLayer() const noexcept { return layer_; }
    [[nodiscard]] uint32_t GetLayerMask() const noexcept { return layerMask_; }
    [[nodiscard]] const Vector3& GetCenter() const noexcept { return center_; }

    void SetIsTrigger(bool trigger) noexcept { isTrigger_ = trigger; }
    void SetLayer(uint32_t layer) noexcept { layer_ = layer; }
    void SetLayerMask(uint32_t mask) noexcept { layerMask_ = mask; }
    void SetCenter(const Vector3& center) noexcept { center_ = center; }

    //! @brief レイヤーマスクで衝突可能か判定
    [[nodiscard]] bool CanCollideWith(const Collider* other) const {
        if (!other) return false;
        return (layerMask_ & (1u << other->layer_)) != 0;
    }

    //------------------------------------------------------------------------
    // コールバック
    //------------------------------------------------------------------------
    std::function<void(const CollisionInfo&)> OnCollisionEnter;
    std::function<void(const CollisionInfo&)> OnCollisionStay;
    std::function<void(const CollisionInfo&)> OnCollisionExit;
    std::function<void(Collider*)> OnTriggerEnter;
    std::function<void(Collider*)> OnTriggerStay;
    std::function<void(Collider*)> OnTriggerExit;

protected:
    [[nodiscard]] Vector3 GetWorldCenter() const {
        if (transform_) {
            return transform_->GetPosition() + center_;
        }
        return center_;
    }

    Transform* transform_ = nullptr;
    Vector3 center_ = Vector3::Zero;
    bool isTrigger_ = false;
    uint32_t layer_ = 0;
    uint32_t layerMask_ = 0xFFFFFFFF;  // デフォルトは全レイヤーと衝突
};

//============================================================================
//! @brief BoxCollider - 3Dボックス当たり判定
//============================================================================
class BoxCollider final : public Collider {
public:
    BoxCollider() = default;

    explicit BoxCollider(const Vector3& size)
        : size_(size) {}

    [[nodiscard]] const Vector3& GetSize() const noexcept { return size_; }
    void SetSize(const Vector3& size) noexcept { size_ = size; }

    [[nodiscard]] bool Intersects(const Collider* other) const override;
    [[nodiscard]] bool ContainsPoint(const Vector3& point) const override;
    [[nodiscard]] bool Raycast(const Vector3& origin, const Vector3& direction,
                                float maxDistance, float& hitDistance) const override;
    void GetWorldBounds(Vector3& min, Vector3& max) const override;

private:
    Vector3 size_ = Vector3::One;
};

//============================================================================
//! @brief SphereCollider - 3D球当たり判定
//============================================================================
class SphereCollider final : public Collider {
public:
    SphereCollider() = default;

    explicit SphereCollider(float radius)
        : radius_(radius) {}

    [[nodiscard]] float GetRadius() const noexcept { return radius_; }
    void SetRadius(float radius) noexcept { radius_ = radius; }

    [[nodiscard]] bool Intersects(const Collider* other) const override;
    [[nodiscard]] bool ContainsPoint(const Vector3& point) const override;
    [[nodiscard]] bool Raycast(const Vector3& origin, const Vector3& direction,
                                float maxDistance, float& hitDistance) const override;
    void GetWorldBounds(Vector3& min, Vector3& max) const override;

private:
    float radius_ = 0.5f;
};

//============================================================================
//! @brief CircleCollider - 2D円当たり判定
//============================================================================
class CircleCollider final : public Collider {
public:
    CircleCollider() = default;

    explicit CircleCollider(float radius)
        : radius_(radius) {}

    [[nodiscard]] float GetRadius() const noexcept { return radius_; }
    void SetRadius(float radius) noexcept { radius_ = radius; }

    [[nodiscard]] bool Intersects(const Collider* other) const override;
    [[nodiscard]] bool ContainsPoint(const Vector3& point) const override;
    [[nodiscard]] bool Raycast(const Vector3& origin, const Vector3& direction,
                                float maxDistance, float& hitDistance) const override;
    void GetWorldBounds(Vector3& min, Vector3& max) const override;

private:
    float radius_ = 0.5f;
};

//============================================================================
//! @brief BoxCollider2D - 2Dボックス当たり判定
//============================================================================
class BoxCollider2D final : public Collider {
public:
    BoxCollider2D() = default;

    BoxCollider2D(float width, float height)
        : width_(width), height_(height) {}

    [[nodiscard]] float GetWidth() const noexcept { return width_; }
    [[nodiscard]] float GetHeight() const noexcept { return height_; }
    void SetSize(float width, float height) noexcept { width_ = width; height_ = height; }

    [[nodiscard]] bool Intersects(const Collider* other) const override;
    [[nodiscard]] bool ContainsPoint(const Vector3& point) const override;
    [[nodiscard]] bool Raycast(const Vector3& origin, const Vector3& direction,
                                float maxDistance, float& hitDistance) const override;
    void GetWorldBounds(Vector3& min, Vector3& max) const override;

private:
    float width_ = 1.0f;
    float height_ = 1.0f;
};

//============================================================================
// 実装
//============================================================================

// BoxCollider
inline bool BoxCollider::Intersects(const Collider* other) const {
    if (auto* box = dynamic_cast<const BoxCollider*>(other)) {
        Vector3 minA, maxA, minB, maxB;
        GetWorldBounds(minA, maxA);
        box->GetWorldBounds(minB, maxB);

        return (minA.x <= maxB.x && maxA.x >= minB.x) &&
               (minA.y <= maxB.y && maxA.y >= minB.y) &&
               (minA.z <= maxB.z && maxA.z >= minB.z);
    }
    if (auto* sphere = dynamic_cast<const SphereCollider*>(other)) {
        // Box vs Sphere
        Vector3 center = sphere->GetWorldCenter();
        float radius = sphere->GetRadius();

        Vector3 minA, maxA;
        GetWorldBounds(minA, maxA);

        Vector3 closest;
        closest.x = std::clamp(center.x, minA.x, maxA.x);
        closest.y = std::clamp(center.y, minA.y, maxA.y);
        closest.z = std::clamp(center.z, minA.z, maxA.z);

        return Vector3::DistanceSquared(center, closest) <= radius * radius;
    }
    return false;
}

inline bool BoxCollider::ContainsPoint(const Vector3& point) const {
    Vector3 min, max;
    GetWorldBounds(min, max);
    return point.x >= min.x && point.x <= max.x &&
           point.y >= min.y && point.y <= max.y &&
           point.z >= min.z && point.z <= max.z;
}

inline bool BoxCollider::Raycast(const Vector3& origin, const Vector3& direction,
                                  float maxDistance, float& hitDistance) const {
    Vector3 min, max;
    GetWorldBounds(min, max);

    float tmin = 0.0f;
    float tmax = maxDistance;

    for (int i = 0; i < 3; ++i) {
        float o = (&origin.x)[i];
        float d = (&direction.x)[i];
        float bmin = (&min.x)[i];
        float bmax = (&max.x)[i];

        if (std::abs(d) < 0.0001f) {
            if (o < bmin || o > bmax) return false;
        } else {
            float t1 = (bmin - o) / d;
            float t2 = (bmax - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }

    hitDistance = tmin;
    return true;
}

inline void BoxCollider::GetWorldBounds(Vector3& min, Vector3& max) const {
    Vector3 center = GetWorldCenter();
    Vector3 halfSize = size_ * 0.5f;
    min = center - halfSize;
    max = center + halfSize;
}

// SphereCollider
inline bool SphereCollider::Intersects(const Collider* other) const {
    if (auto* sphere = dynamic_cast<const SphereCollider*>(other)) {
        float dist = Vector3::Distance(GetWorldCenter(), sphere->GetWorldCenter());
        return dist <= (radius_ + sphere->radius_);
    }
    if (auto* box = dynamic_cast<const BoxCollider*>(other)) {
        return box->Intersects(this);
    }
    return false;
}

inline bool SphereCollider::ContainsPoint(const Vector3& point) const {
    return Vector3::DistanceSquared(GetWorldCenter(), point) <= radius_ * radius_;
}

inline bool SphereCollider::Raycast(const Vector3& origin, const Vector3& direction,
                                     float maxDistance, float& hitDistance) const {
    Vector3 center = GetWorldCenter();
    Vector3 oc = origin - center;

    float a = direction.Dot(direction);
    float b = 2.0f * oc.Dot(direction);
    float c = oc.Dot(oc) - radius_ * radius_;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    if (t < 0 || t > maxDistance) {
        t = (-b + std::sqrt(discriminant)) / (2.0f * a);
        if (t < 0 || t > maxDistance) return false;
    }

    hitDistance = t;
    return true;
}

inline void SphereCollider::GetWorldBounds(Vector3& min, Vector3& max) const {
    Vector3 center = GetWorldCenter();
    min = center - Vector3(radius_, radius_, radius_);
    max = center + Vector3(radius_, radius_, radius_);
}

// CircleCollider (2D - uses X and Y)
inline bool CircleCollider::Intersects(const Collider* other) const {
    if (auto* circle = dynamic_cast<const CircleCollider*>(other)) {
        Vector3 a = GetWorldCenter();
        Vector3 b = circle->GetWorldCenter();
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        return dist <= (radius_ + circle->radius_);
    }
    if (auto* box2d = dynamic_cast<const BoxCollider2D*>(other)) {
        return box2d->Intersects(this);
    }
    return false;
}

inline bool CircleCollider::ContainsPoint(const Vector3& point) const {
    Vector3 center = GetWorldCenter();
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    return (dx * dx + dy * dy) <= radius_ * radius_;
}

inline bool CircleCollider::Raycast(const Vector3& origin, const Vector3& direction,
                                     float maxDistance, float& hitDistance) const {
    // 2Dレイキャスト（Z無視）
    Vector3 center = GetWorldCenter();
    Vector2 oc(origin.x - center.x, origin.y - center.y);
    Vector2 dir(direction.x, direction.y);

    float a = dir.Dot(dir);
    float b = 2.0f * oc.Dot(dir);
    float c = oc.Dot(oc) - radius_ * radius_;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    if (t < 0 || t > maxDistance) return false;

    hitDistance = t;
    return true;
}

inline void CircleCollider::GetWorldBounds(Vector3& min, Vector3& max) const {
    Vector3 center = GetWorldCenter();
    min = Vector3(center.x - radius_, center.y - radius_, 0);
    max = Vector3(center.x + radius_, center.y + radius_, 0);
}

// BoxCollider2D
inline bool BoxCollider2D::Intersects(const Collider* other) const {
    if (auto* box2d = dynamic_cast<const BoxCollider2D*>(other)) {
        Vector3 minA, maxA, minB, maxB;
        GetWorldBounds(minA, maxA);
        box2d->GetWorldBounds(minB, maxB);

        return (minA.x <= maxB.x && maxA.x >= minB.x) &&
               (minA.y <= maxB.y && maxA.y >= minB.y);
    }
    if (auto* circle = dynamic_cast<const CircleCollider*>(other)) {
        Vector3 center = circle->GetWorldCenter();
        float radius = circle->GetRadius();

        Vector3 minA, maxA;
        GetWorldBounds(minA, maxA);

        float closestX = std::clamp(center.x, minA.x, maxA.x);
        float closestY = std::clamp(center.y, minA.y, maxA.y);

        float dx = center.x - closestX;
        float dy = center.y - closestY;
        return (dx * dx + dy * dy) <= radius * radius;
    }
    return false;
}

inline bool BoxCollider2D::ContainsPoint(const Vector3& point) const {
    Vector3 min, max;
    GetWorldBounds(min, max);
    return point.x >= min.x && point.x <= max.x &&
           point.y >= min.y && point.y <= max.y;
}

inline bool BoxCollider2D::Raycast(const Vector3& origin, const Vector3& direction,
                                    float maxDistance, float& hitDistance) const {
    Vector3 min, max;
    GetWorldBounds(min, max);

    float tmin = 0.0f;
    float tmax = maxDistance;

    // X軸
    if (std::abs(direction.x) < 0.0001f) {
        if (origin.x < min.x || origin.x > max.x) return false;
    } else {
        float t1 = (min.x - origin.x) / direction.x;
        float t2 = (max.x - origin.x) / direction.x;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Y軸
    if (std::abs(direction.y) < 0.0001f) {
        if (origin.y < min.y || origin.y > max.y) return false;
    } else {
        float t1 = (min.y - origin.y) / direction.y;
        float t2 = (max.y - origin.y) / direction.y;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    hitDistance = tmin;
    return true;
}

inline void BoxCollider2D::GetWorldBounds(Vector3& min, Vector3& max) const {
    Vector3 center = GetWorldCenter();
    min = Vector3(center.x - width_ * 0.5f, center.y - height_ * 0.5f, 0);
    max = Vector3(center.x + width_ * 0.5f, center.y + height_ * 0.5f, 0);
}

OOP_COMPONENT(BoxCollider);
OOP_COMPONENT(SphereCollider);
OOP_COMPONENT(CircleCollider);
OOP_COMPONENT(BoxCollider2D);
