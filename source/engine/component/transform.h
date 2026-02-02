//----------------------------------------------------------------------------
//! @file   transform.h
//! @brief  トランスフォームコンポーネント（3D基準）
//----------------------------------------------------------------------------
#pragma once

#include "component.h"
#include "engine/math/math_types.h"
#include <vector>
#include <algorithm>

//============================================================================
//! @brief トランスフォームコンポーネント
//!
//! 3D空間での位置・回転・スケールを管理する。
//! 親子階層をサポートし、ワールド座標系の変換機能を提供。
//============================================================================
class Transform : public Component {
public:
    Transform() = default;

    ~Transform() {
        if (parent_) {
            auto& siblings = parent_->children_;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
            parent_ = nullptr;
        }
        for (Transform* child : children_) {
            child->parent_ = nullptr;
        }
        children_.clear();
    }

    explicit Transform(const Vector3& position)
        : position_(position) {}

    Transform(const Vector3& position, const Quaternion& rotation)
        : position_(position), rotation_(rotation) {}

    Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
        : position_(position), rotation_(rotation), scale_(scale) {}

    //------------------------------------------------------------------------
    // 位置
    //------------------------------------------------------------------------

    [[nodiscard]] const Vector3& GetPosition() const noexcept { return position_; }

    void SetPosition(const Vector3& position) noexcept {
        position_ = position;
        SetDirty();
    }

    void SetPosition(float x, float y, float z) noexcept {
        position_ = Vector3(x, y, z);
        SetDirty();
    }

    void Translate(const Vector3& delta) noexcept {
        position_ += delta;
        SetDirty();
    }

    void Translate(float dx, float dy, float dz) noexcept {
        position_.x += dx;
        position_.y += dy;
        position_.z += dz;
        SetDirty();
    }

    //------------------------------------------------------------------------
    // 回転
    //------------------------------------------------------------------------

    [[nodiscard]] const Quaternion& GetRotation() const noexcept { return rotation_; }

    void SetRotation(const Quaternion& rotation) noexcept {
        rotation_ = rotation;
        SetDirty();
    }

    void Rotate(const Vector3& axis, float angle) noexcept {
        Quaternion delta = Quaternion::CreateFromAxisAngle(axis, angle);
        rotation_ = rotation_ * delta;
        SetDirty();
    }

    //------------------------------------------------------------------------
    // スケール
    //------------------------------------------------------------------------

    [[nodiscard]] const Vector3& GetScale() const noexcept { return scale_; }

    void SetScale(const Vector3& scale) noexcept {
        scale_ = scale;
        SetDirty();
    }

    void SetScale(float uniformScale) noexcept {
        scale_ = Vector3(uniformScale, uniformScale, uniformScale);
        SetDirty();
    }

    //------------------------------------------------------------------------
    // 方向ベクトル
    //------------------------------------------------------------------------

    [[nodiscard]] Vector3 GetForward() const noexcept {
        return Vector3::Transform(LH::Forward(), rotation_);
    }

    [[nodiscard]] Vector3 GetRight() const noexcept {
        return Vector3::Transform(Vector3::Right, rotation_);
    }

    [[nodiscard]] Vector3 GetUp() const noexcept {
        return Vector3::Transform(Vector3::Up, rotation_);
    }

    //------------------------------------------------------------------------
    // 親子階層
    //------------------------------------------------------------------------

    [[nodiscard]] Transform* GetParent() const noexcept { return parent_; }

    void SetParent(Transform* parent) {
        if (parent_ == parent) return;

        if (parent) {
            Transform* p = parent;
            while (p) {
                if (p == this) return;
                p = p->parent_;
            }
        }

        if (parent_) {
            auto& siblings = parent_->children_;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
        }

        parent_ = parent;
        if (parent_) {
            parent_->children_.push_back(this);
        }

        SetDirty();
    }

    void AddChild(Transform* child) {
        if (child && child != this) {
            child->SetParent(this);
        }
    }

    void RemoveChild(Transform* child) {
        if (child && child->parent_ == this) {
            child->SetParent(nullptr);
        }
    }

    [[nodiscard]] const std::vector<Transform*>& GetChildren() const noexcept { return children_; }
    [[nodiscard]] size_t GetChildCount() const noexcept { return children_.size(); }

    void DetachFromParent() { SetParent(nullptr); }

    void DetachAllChildren() {
        while (!children_.empty()) {
            children_.back()->SetParent(nullptr);
        }
    }

    //------------------------------------------------------------------------
    // ワールド座標
    //------------------------------------------------------------------------

    [[nodiscard]] Vector3 GetWorldPosition() {
        if (parent_) {
            return Vector3::Transform(position_, parent_->GetWorldMatrix());
        }
        return position_;
    }

    [[nodiscard]] Quaternion GetWorldRotation() {
        if (parent_) {
            return rotation_ * parent_->GetWorldRotation();
        }
        return rotation_;
    }

    [[nodiscard]] Vector3 GetWorldScale() {
        if (parent_) {
            Vector3 parentScale = parent_->GetWorldScale();
            return Vector3(scale_.x * parentScale.x, scale_.y * parentScale.y, scale_.z * parentScale.z);
        }
        return scale_;
    }

    void SetWorldPosition(const Vector3& worldPos) {
        if (parent_) {
            Matrix invParent = parent_->GetWorldMatrix().Invert();
            SetPosition(Vector3::Transform(worldPos, invParent));
        } else {
            SetPosition(worldPos);
        }
    }

    //------------------------------------------------------------------------
    // ワールド行列
    //------------------------------------------------------------------------

    [[nodiscard]] const Matrix& GetWorldMatrix() {
        if (dirty_) {
            UpdateWorldMatrix();
        }
        return worldMatrix_;
    }

    void ForceUpdateMatrix() {
        dirty_ = true;
    }

private:
    void SetDirty() noexcept {
        if (dirty_) return;
        dirty_ = true;
        for (Transform* child : children_) {
            child->SetDirty();
        }
    }

    void UpdateWorldMatrix() {
        Matrix localMatrix = Matrix::CreateScale(scale_)
                           * Matrix::CreateFromQuaternion(rotation_)
                           * Matrix::CreateTranslation(position_);

        if (parent_) {
            worldMatrix_ = localMatrix * parent_->GetWorldMatrix();
        } else {
            worldMatrix_ = localMatrix;
        }

        dirty_ = false;
    }

    // ローカル変換
    Vector3 position_ = Vector3::Zero;
    Quaternion rotation_ = Quaternion::Identity;
    Vector3 scale_ = Vector3::One;

    // 階層構造
    Transform* parent_ = nullptr;
    std::vector<Transform*> children_;

    // キャッシュ
    Matrix worldMatrix_ = Matrix::Identity;
    bool dirty_ = true;
};
