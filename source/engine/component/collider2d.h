//----------------------------------------------------------------------------
//! @file   collider2d.h
//! @brief  2D当たり判定コンポーネント（AABB）
//----------------------------------------------------------------------------
#pragma once

#include "component.h"
#include "engine/math/math_types.h"
#include <functional>
#include <cstdint>

class Collider2D;

//============================================================================
//! @brief AABB（軸平行境界ボックス）
//============================================================================
struct AABB {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;

    AABB() = default;
    AABB(float x, float y, float w, float h)
        : minX(x), minY(y), maxX(x + w), maxY(y + h) {}

    [[nodiscard]] bool Intersects(const AABB& other) const noexcept {
        return minX < other.maxX && maxX > other.minX &&
               minY < other.maxY && maxY > other.minY;
    }

    [[nodiscard]] bool Contains(float px, float py) const noexcept {
        return px >= minX && px < maxX && py >= minY && py < maxY;
    }

    [[nodiscard]] Vector2 GetCenter() const noexcept {
        return Vector2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);
    }

    [[nodiscard]] Vector2 GetSize() const noexcept {
        return Vector2(maxX - minX, maxY - minY);
    }
};

//============================================================================
//! @brief 衝突コールバック型
//============================================================================
using CollisionCallback = std::function<void(Collider2D*, Collider2D*)>;

//============================================================================
//! @brief 2D当たり判定コンポーネント（AABB）
//!
//! GameObjectにアタッチして当たり判定を追加する。
//! Unity風の設計: コンポーネントがデータを保持し、
//! CollisionManagerは参照して衝突検出を行う。
//============================================================================
class Collider2D : public Component {
public:
    Collider2D() = default;

    explicit Collider2D(const Vector2& size, const Vector2& offset = Vector2::Zero)
        : size_(size), offset_(offset) {}

    //------------------------------------------------------------------------
    // Component オーバーライド
    //------------------------------------------------------------------------

    void OnAttach() override;
    void OnDetach() override;
    void Update(float deltaTime) override;

    //------------------------------------------------------------------------
    // 位置
    //------------------------------------------------------------------------

    void SetPosition(float x, float y) noexcept {
        position_ = Vector2(x, y);
        syncWithTransform_ = false;
    }

    void SetPosition(const Vector2& pos) noexcept {
        position_ = pos;
        syncWithTransform_ = false;
    }

    [[nodiscard]] const Vector2& GetPosition() const noexcept { return position_; }

    //------------------------------------------------------------------------
    // サイズとオフセット
    //------------------------------------------------------------------------

    void SetSize(float width, float height) noexcept { size_ = Vector2(width, height); }
    void SetSize(const Vector2& size) noexcept { size_ = size; }
    [[nodiscard]] const Vector2& GetSize() const noexcept { return size_; }

    void SetOffset(float x, float y) noexcept { offset_ = Vector2(x, y); }
    void SetOffset(const Vector2& offset) noexcept { offset_ = offset; }
    [[nodiscard]] const Vector2& GetOffset() const noexcept { return offset_; }

    void SetBounds(const Vector2& min, const Vector2& max) noexcept {
        size_ = Vector2(max.x - min.x, max.y - min.y);
        offset_ = Vector2(min.x + size_.x * 0.5f, min.y + size_.y * 0.5f);
    }

    //------------------------------------------------------------------------
    // レイヤーとマスク
    //------------------------------------------------------------------------

    void SetLayer(uint8_t layer) noexcept { layer_ = layer; }
    [[nodiscard]] uint8_t GetLayer() const noexcept { return layer_; }

    void SetMask(uint8_t mask) noexcept { mask_ = mask; }
    [[nodiscard]] uint8_t GetMask() const noexcept { return mask_; }

    [[nodiscard]] bool CanCollideWith(uint8_t otherLayer) const noexcept {
        return (mask_ & otherLayer) != 0;
    }

    //------------------------------------------------------------------------
    // トリガーモード
    //------------------------------------------------------------------------

    void SetTrigger(bool trigger) noexcept { trigger_ = trigger; }
    [[nodiscard]] bool IsTrigger() const noexcept { return trigger_; }

    //------------------------------------------------------------------------
    // 有効/無効
    //------------------------------------------------------------------------

    void SetColliderEnabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool IsColliderEnabled() const noexcept { return enabled_; }

    //------------------------------------------------------------------------
    // AABB取得
    //------------------------------------------------------------------------

    [[nodiscard]] AABB GetAABB() const noexcept {
        float halfW = size_.x * 0.5f;
        float halfH = size_.y * 0.5f;
        float cx = position_.x + offset_.x;
        float cy = position_.y + offset_.y;
        return AABB(cx - halfW, cy - halfH, size_.x, size_.y);
    }

    //------------------------------------------------------------------------
    // 衝突コールバック
    //------------------------------------------------------------------------

    void SetOnCollision(CollisionCallback callback) { onCollision_ = std::move(callback); }
    void SetOnCollisionEnter(CollisionCallback callback) { onEnter_ = std::move(callback); }
    void SetOnCollisionExit(CollisionCallback callback) { onExit_ = std::move(callback); }

    // Manager用：コールバック呼び出し
    void InvokeOnCollision(Collider2D* other) { if (onCollision_) onCollision_(this, other); }
    void InvokeOnEnter(Collider2D* other) { if (onEnter_) onEnter_(this, other); }
    void InvokeOnExit(Collider2D* other) { if (onExit_) onExit_(this, other); }

    //------------------------------------------------------------------------
    // ユーザーデータ
    //------------------------------------------------------------------------

    void SetUserData(void* data) noexcept { userData_ = data; }
    [[nodiscard]] void* GetUserData() const noexcept { return userData_; }

    template<typename T>
    void SetUserData(T* data) noexcept { userData_ = static_cast<void*>(data); }

    template<typename T>
    [[nodiscard]] T* GetUserDataAs() const noexcept { return static_cast<T*>(userData_); }

    //------------------------------------------------------------------------
    // Transform同期設定
    //------------------------------------------------------------------------

    void SetSyncWithTransform(bool sync) noexcept { syncWithTransform_ = sync; }
    [[nodiscard]] bool IsSyncWithTransform() const noexcept { return syncWithTransform_; }

private:
    Vector2 position_ = Vector2::Zero;
    Vector2 size_ = Vector2::Zero;
    Vector2 offset_ = Vector2::Zero;

    uint8_t layer_ = 0x01;
    uint8_t mask_ = 0xFF;
    bool trigger_ = false;
    bool enabled_ = true;
    bool syncWithTransform_ = true;

    CollisionCallback onCollision_;
    CollisionCallback onEnter_;
    CollisionCallback onExit_;

    void* userData_ = nullptr;
};
