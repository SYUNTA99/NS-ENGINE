//----------------------------------------------------------------------------
//! @file   character_controller.h
//! @brief  CharacterController - キャラクター移動コンポーネント
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component.h"
#include "engine/math/math_types.h"
#include "engine/physics/mesh_collider.h"
#include "engine/physics/raycast.h"
#include "transform.h"
#include <functional>

//============================================================================
//! @brief CharacterController - Unity風キャラクター移動コンポーネント
//!
//! 重力、地面検出、移動を管理する。MeshColliderと連携してレイキャスト
//! ベースの地面判定を行う。
//!
//! @code
//! auto* go = world.CreateGameObject("Player");
//! go->AddComponent<Transform>(Vector3(0, 10, 0));
//! auto* cc = go->AddComponent<CharacterController>();
//!
//! cc->SetGroundCollider(stageMeshCollider);
//! cc->Move(Vector3(1, 0, 0) * speed * dt);
//! @endcode
//============================================================================
class CharacterController final : public Component {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    CharacterController() = default;

    explicit CharacterController(float height, float radius)
        : height_(height), radius_(radius) {}

    //------------------------------------------------------------------------
    // ライフサイクル
    //------------------------------------------------------------------------
    void Start() override {
        transform_ = GetComponent<Transform>();
    }

    void FixedUpdate(float dt) override {
        if (!transform_) {
            transform_ = GetComponent<Transform>();
            if (!transform_) return;
        }

        // 重力適用
        if (!isGrounded_ && useGravity_) {
            velocity_.y += gravity_ * dt;
        }

        // 速度による移動
        if (velocity_.LengthSquared() > 0.0001f) {
            Vector3 pos = transform_->GetPosition();
            pos += velocity_ * dt;
            transform_->SetPosition(pos);
        }

        // 地面検出
        UpdateGroundCheck();

        // 着地処理
        if (isGrounded_ && velocity_.y < 0) {
            velocity_.y = 0;

            // 地面にスナップ
            if (lastGroundHit_.hit) {
                Vector3 pos = transform_->GetPosition();
                pos.y = lastGroundHit_.point.y + groundOffset_;
                transform_->SetPosition(pos);
            }
        }
    }

    //========================================================================
    // 移動
    //========================================================================

    //! @brief 移動（ワールド座標）
    void Move(const Vector3& motion) {
        if (!transform_) return;
        Vector3 pos = transform_->GetPosition();
        pos += motion;
        transform_->SetPosition(pos);
    }

    //! @brief 相対移動（前後左右）
    void SimpleMove(const Vector3& speed) {
        velocity_.x = speed.x;
        velocity_.z = speed.z;
        // Y速度は重力で制御
    }

    //! @brief ジャンプ
    void Jump(float force) {
        if (isGrounded_) {
            velocity_.y = force;
            isGrounded_ = false;
        }
    }

    //! @brief 強制ジャンプ（地面判定無視）
    void ForceJump(float force) {
        velocity_.y = force;
        isGrounded_ = false;
    }

    //========================================================================
    // 地面コライダー設定
    //========================================================================

    //! @brief 地面検出用MeshColliderを設定
    void SetGroundCollider(Physics::MeshColliderPtr collider) {
        groundCollider_ = collider;
    }

    //! @brief 地面検出用MeshColliderを追加
    void AddGroundCollider(Physics::MeshColliderPtr collider) {
        groundColliders_.push_back(collider);
    }

    //! @brief 地面コライダーをクリア
    void ClearGroundColliders() {
        groundCollider_ = nullptr;
        groundColliders_.clear();
    }

    //========================================================================
    // 状態取得
    //========================================================================

    [[nodiscard]] bool IsGrounded() const noexcept { return isGrounded_; }
    [[nodiscard]] const Vector3& GetVelocity() const noexcept { return velocity_; }
    [[nodiscard]] const Physics::RaycastHit& GetGroundHit() const noexcept { return lastGroundHit_; }

    void SetVelocity(const Vector3& vel) noexcept { velocity_ = vel; }

    //========================================================================
    // パラメータ
    //========================================================================

    [[nodiscard]] float GetHeight() const noexcept { return height_; }
    [[nodiscard]] float GetRadius() const noexcept { return radius_; }
    [[nodiscard]] float GetGravity() const noexcept { return gravity_; }
    [[nodiscard]] float GetGroundCheckDistance() const noexcept { return groundCheckDistance_; }
    [[nodiscard]] float GetSlopeLimit() const noexcept { return slopeLimit_; }
    [[nodiscard]] bool GetUseGravity() const noexcept { return useGravity_; }

    void SetHeight(float h) noexcept { height_ = h; }
    void SetRadius(float r) noexcept { radius_ = r; }
    void SetGravity(float g) noexcept { gravity_ = g; }
    void SetGroundCheckDistance(float d) noexcept { groundCheckDistance_ = d; }
    void SetSlopeLimit(float deg) noexcept { slopeLimit_ = deg; }
    void SetUseGravity(bool use) noexcept { useGravity_ = use; }
    void SetGroundOffset(float offset) noexcept { groundOffset_ = offset; }

    //========================================================================
    // コールバック
    //========================================================================
    std::function<void()> OnLanded;              //!< 着地時
    std::function<void()> OnBecameAirborne;      //!< 空中になった時

private:
    void UpdateGroundCheck() {
        if (!transform_) return;

        bool wasGrounded = isGrounded_;
        isGrounded_ = false;
        lastGroundHit_.Reset();

        Vector3 origin = transform_->GetPosition();
        origin.y += 0.1f; // 少し上から

        Physics::Ray ray(origin, Vector3(0, -1, 0));
        float checkDist = groundCheckDistance_ + 0.2f;

        // メインコライダーでチェック
        if (groundCollider_) {
            Physics::RaycastHit hit;
            if (groundCollider_->Raycast(ray, checkDist, hit)) {
                if (hit.distance <= groundCheckDistance_ + 0.1f) {
                    // 斜面チェック
                    float angle = std::acos(hit.normal.Dot(Vector3::Up)) * (180.0f / 3.14159265f);
                    if (angle <= slopeLimit_) {
                        isGrounded_ = true;
                        lastGroundHit_ = hit;
                    }
                }
            }
        }

        // 追加コライダーでチェック
        for (const auto& collider : groundColliders_) {
            if (!collider) continue;

            Physics::RaycastHit hit;
            if (collider->Raycast(ray, checkDist, hit)) {
                if (hit.distance <= groundCheckDistance_ + 0.1f) {
                    float angle = std::acos(hit.normal.Dot(Vector3::Up)) * (180.0f / 3.14159265f);
                    if (angle <= slopeLimit_) {
                        if (!isGrounded_ || hit.distance < lastGroundHit_.distance) {
                            isGrounded_ = true;
                            lastGroundHit_ = hit;
                        }
                    }
                }
            }
        }

        // コールバック
        if (isGrounded_ && !wasGrounded && OnLanded) {
            OnLanded();
        }
        if (!isGrounded_ && wasGrounded && OnBecameAirborne) {
            OnBecameAirborne();
        }
    }

    Transform* transform_ = nullptr;

    // 形状
    float height_ = 2.0f;
    float radius_ = 0.5f;

    // 物理
    Vector3 velocity_ = Vector3::Zero;
    float gravity_ = -20.0f;
    bool useGravity_ = true;

    // 地面検出
    float groundCheckDistance_ = 0.3f;
    float groundOffset_ = 0.0f;      //!< 地面からのオフセット
    float slopeLimit_ = 45.0f;       //!< 歩行可能な斜面の角度
    bool isGrounded_ = false;
    Physics::RaycastHit lastGroundHit_;

    // コライダー
    Physics::MeshColliderPtr groundCollider_;
    std::vector<Physics::MeshColliderPtr> groundColliders_;
};

OOP_COMPONENT(CharacterController);
