//----------------------------------------------------------------------------
//! @file   rigidbody.h
//! @brief  Rigidbody - 純粋OOP物理コンポーネント
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component.h"
#include "engine/math/math_types.h"
#include "transform.h"
#include <functional>
#include <algorithm>

//============================================================================
//! @brief Rigidbody - 物理挙動を管理する純粋OOPコンポーネント
//!
//! Unity MonoBehaviour風の設計。速度、重力、力の適用を管理。
//!
//! @code
//! auto* go = world.CreateGameObject("Ball");
//! go->AddComponent<Transform>(Vector3(0, 10, 0));
//! auto* rb = go->AddComponent<Rigidbody>();
//!
//! rb->SetMass(1.0f);
//! rb->SetUseGravity(true);
//! rb->AddForce(LH::Forward() * 100.0f);
//! rb->AddImpulse(Vector3::Up * 500.0f);
//! @endcode
//============================================================================
class Rigidbody final : public Component {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Rigidbody() = default;

    explicit Rigidbody(float mass)
        : mass_(mass) {}

    //------------------------------------------------------------------------
    // ライフサイクル
    //------------------------------------------------------------------------
    void Start() override {
        // Transformを取得
        transform_ = GetComponent<Transform>();
    }

    void FixedUpdate(float dt) override {
        if (isKinematic_) return;
        if (!transform_) {
            transform_ = GetComponent<Transform>();
            if (!transform_) return;
        }

        // 重力適用
        if (useGravity_) {
            velocity_ += gravity_ * dt;
        }

        // 力を加速度に変換して速度に加算
        if (mass_ > 0.0f) {
            velocity_ += (accumulatedForce_ / mass_) * dt;
        }
        accumulatedForce_ = Vector3::Zero;

        // 角速度適用
        if (angularVelocity_.LengthSquared() > 0.0001f) {
            Vector3 euler = transform_->GetEulerAngles();
            euler += angularVelocity_ * dt * (180.0f / 3.14159265f);
            transform_->SetEulerAngles(euler);
        }

        // 抵抗適用
        velocity_ *= (1.0f - drag_ * dt);
        angularVelocity_ *= (1.0f - angularDrag_ * dt);

        // 速度制限
        float speed = velocity_.Length();
        if (speed > maxVelocity_) {
            velocity_ = (velocity_ / speed) * maxVelocity_;
        }

        // 位置更新
        Vector3 pos = transform_->GetPosition();
        pos += velocity_ * dt;
        transform_->SetPosition(pos);

        // スリープ判定
        if (velocity_.LengthSquared() < sleepThreshold_ * sleepThreshold_ &&
            angularVelocity_.LengthSquared() < sleepThreshold_ * sleepThreshold_) {
            sleepTimer_ += dt;
            if (sleepTimer_ > sleepDelay_) {
                isSleeping_ = true;
            }
        } else {
            sleepTimer_ = 0.0f;
            isSleeping_ = false;
        }
    }

    //========================================================================
    // 力の適用
    //========================================================================

    //! @brief 力を加える（質量を考慮）
    void AddForce(const Vector3& force) {
        accumulatedForce_ += force;
        WakeUp();
    }

    //! @brief 瞬間的な力を加える（インパルス）
    void AddImpulse(const Vector3& impulse) {
        if (mass_ > 0.0f) {
            velocity_ += impulse / mass_;
        }
        WakeUp();
    }

    //! @brief 位置に力を加える（トルクも発生）
    void AddForceAtPosition(const Vector3& force, const Vector3& position) {
        AddForce(force);
        // トルク = r × F
        if (transform_) {
            Vector3 r = position - transform_->GetPosition();
            Vector3 torque = r.Cross(force);
            AddTorque(torque);
        }
    }

    //! @brief トルク（回転力）を加える
    void AddTorque(const Vector3& torque) {
        if (mass_ > 0.0f) {
            angularVelocity_ += torque / mass_;
        }
        WakeUp();
    }

    //! @brief 特定の方向に爆発的な力を加える
    void AddExplosionForce(float force, const Vector3& explosionPosition, float radius) {
        if (!transform_) return;

        Vector3 dir = transform_->GetPosition() - explosionPosition;
        float distance = dir.Length();

        if (distance < radius && distance > 0.001f) {
            dir.Normalize();
            float falloff = 1.0f - (distance / radius);
            AddImpulse(dir * force * falloff);
        }
    }

    //========================================================================
    // 速度制御
    //========================================================================

    [[nodiscard]] const Vector3& GetVelocity() const noexcept { return velocity_; }
    [[nodiscard]] const Vector3& GetAngularVelocity() const noexcept { return angularVelocity_; }

    void SetVelocity(const Vector3& velocity) {
        velocity_ = velocity;
        WakeUp();
    }

    void SetAngularVelocity(const Vector3& angularVelocity) {
        angularVelocity_ = angularVelocity;
        WakeUp();
    }

    //! @brief 速度をゼロにする
    void Stop() {
        velocity_ = Vector3::Zero;
        angularVelocity_ = Vector3::Zero;
    }

    //! @brief 特定方向の速度を取得
    [[nodiscard]] float GetSpeedInDirection(const Vector3& direction) const {
        Vector3 dir = direction;
        dir.Normalize();
        return velocity_.Dot(dir);
    }

    //========================================================================
    // 移動
    //========================================================================

    //! @brief 目標位置に向かって移動（Kinematic用）
    void MovePosition(const Vector3& position) {
        if (transform_ && isKinematic_) {
            transform_->SetPosition(position);
        }
    }

    //! @brief 目標回転に向かって回転（Kinematic用）
    void MoveRotation(const Quaternion& rotation) {
        if (transform_ && isKinematic_) {
            transform_->SetRotation(rotation);
        }
    }

    //========================================================================
    // プロパティ
    //========================================================================

    [[nodiscard]] float GetMass() const noexcept { return mass_; }
    [[nodiscard]] float GetDrag() const noexcept { return drag_; }
    [[nodiscard]] float GetAngularDrag() const noexcept { return angularDrag_; }
    [[nodiscard]] bool GetUseGravity() const noexcept { return useGravity_; }
    [[nodiscard]] bool IsKinematic() const noexcept { return isKinematic_; }
    [[nodiscard]] bool IsSleeping() const noexcept { return isSleeping_; }
    [[nodiscard]] const Vector3& GetGravity() const noexcept { return gravity_; }

    void SetMass(float mass) noexcept { mass_ = std::max(0.001f, mass); }
    void SetDrag(float drag) noexcept { drag_ = std::max(0.0f, drag); }
    void SetAngularDrag(float drag) noexcept { angularDrag_ = std::max(0.0f, drag); }
    void SetUseGravity(bool use) noexcept { useGravity_ = use; }
    void SetIsKinematic(bool kinematic) noexcept { isKinematic_ = kinematic; }
    void SetGravity(const Vector3& gravity) noexcept { gravity_ = gravity; }
    void SetMaxVelocity(float max) noexcept { maxVelocity_ = max; }
    void SetSleepThreshold(float threshold) noexcept { sleepThreshold_ = threshold; }

    //! @brief 制約設定
    void FreezePositionX(bool freeze) noexcept { freezePositionX_ = freeze; }
    void FreezePositionY(bool freeze) noexcept { freezePositionY_ = freeze; }
    void FreezePositionZ(bool freeze) noexcept { freezePositionZ_ = freeze; }
    void FreezeRotationX(bool freeze) noexcept { freezeRotationX_ = freeze; }
    void FreezeRotationY(bool freeze) noexcept { freezeRotationY_ = freeze; }
    void FreezeRotationZ(bool freeze) noexcept { freezeRotationZ_ = freeze; }

    //! @brief スリープ状態を解除
    void WakeUp() {
        isSleeping_ = false;
        sleepTimer_ = 0.0f;
    }

    //! @brief スリープ状態にする
    void Sleep() {
        isSleeping_ = true;
        velocity_ = Vector3::Zero;
        angularVelocity_ = Vector3::Zero;
    }

    //========================================================================
    // コールバック
    //========================================================================
    std::function<void(class Collider*)> OnCollisionEnter;
    std::function<void(class Collider*)> OnCollisionStay;
    std::function<void(class Collider*)> OnCollisionExit;

private:
    Transform* transform_ = nullptr;

    // 速度
    Vector3 velocity_ = Vector3::Zero;
    Vector3 angularVelocity_ = Vector3::Zero;
    Vector3 accumulatedForce_ = Vector3::Zero;

    // 物理パラメータ
    float mass_ = 1.0f;
    float drag_ = 0.0f;
    float angularDrag_ = 0.05f;
    float maxVelocity_ = 1000.0f;
    Vector3 gravity_ = Vector3(0.0f, -9.81f, 0.0f);

    // 状態
    bool useGravity_ = true;
    bool isKinematic_ = false;
    bool isSleeping_ = false;
    float sleepTimer_ = 0.0f;
    float sleepThreshold_ = 0.005f;
    float sleepDelay_ = 0.5f;

    // 制約
    bool freezePositionX_ = false;
    bool freezePositionY_ = false;
    bool freezePositionZ_ = false;
    bool freezeRotationX_ = false;
    bool freezeRotationY_ = false;
    bool freezeRotationZ_ = false;
};

OOP_COMPONENT(Rigidbody);
