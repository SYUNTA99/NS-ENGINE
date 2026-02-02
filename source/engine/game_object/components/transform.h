//----------------------------------------------------------------------------
//! @file   transform.h
//! @brief  Transform - 純粋OOPトランスフォームコンポーネント
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component.h"
#include "engine/math/math_types.h"
#include <functional>

//============================================================================
//! @brief Transform - 位置・回転・スケールを管理する純粋OOPコンポーネント
//!
//! Unity MonoBehaviour風の設計。データとロジックが一体化。
//!
//! @code
//! auto* go = world.CreateGameObject("Player");
//! auto* transform = go->AddComponent<Transform>();
//!
//! transform->SetPosition(Vector3(10, 0, 5));
//! transform->Translate(LH::Forward() * speed * dt);
//! transform->Rotate(Vector3::Up, 90.0f);
//! transform->LookAt(targetPos);
//!
//! Vector3 forward = transform->GetForward();
//! Matrix world = transform->GetWorldMatrix();
//! @endcode
//============================================================================
class Transform final : public Component {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Transform() = default;

    Transform(const Vector3& position)
        : position_(position) {}

    Transform(const Vector3& position, const Quaternion& rotation)
        : position_(position), rotation_(rotation) {}

    Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
        : position_(position), rotation_(rotation), scale_(scale) {}

    //------------------------------------------------------------------------
    // ライフサイクル
    //------------------------------------------------------------------------
    void LateUpdate(float dt) override {
        // ワールド行列の再計算（必要な場合のみ）
        if (isDirty_) {
            UpdateWorldMatrix();
        }
    }

    //========================================================================
    // 位置
    //========================================================================

    [[nodiscard]] const Vector3& GetPosition() const noexcept { return position_; }
    [[nodiscard]] const Vector3& GetLocalPosition() const noexcept { return position_; }

    void SetPosition(const Vector3& position) {
        position_ = position;
        MarkDirty();
    }

    void SetPosition(float x, float y, float z) {
        SetPosition(Vector3(x, y, z));
    }

    //! @brief 相対移動（ローカル座標系）
    void Translate(const Vector3& translation) {
        position_ += translation;
        MarkDirty();
    }

    //! @brief 相対移動（ワールド座標系）
    void TranslateWorld(const Vector3& translation) {
        position_ += translation;
        MarkDirty();
    }

    //! @brief 相対移動（自身のローカル座標系基準）
    void TranslateLocal(const Vector3& translation) {
        // 回転を適用した方向に移動
        Vector3 worldTranslation = Vector3::Transform(translation, rotation_);
        position_ += worldTranslation;
        MarkDirty();
    }

    //========================================================================
    // 回転
    //========================================================================

    [[nodiscard]] const Quaternion& GetRotation() const noexcept { return rotation_; }
    [[nodiscard]] const Quaternion& GetLocalRotation() const noexcept { return rotation_; }

    //! @brief オイラー角で取得（度）
    [[nodiscard]] Vector3 GetEulerAngles() const {
        return QuaternionToEuler(rotation_);
    }

    void SetRotation(const Quaternion& rotation) {
        rotation_ = rotation;
        rotation_.Normalize();
        MarkDirty();
    }

    //! @brief オイラー角で設定（度）
    void SetEulerAngles(const Vector3& eulerDegrees) {
        Vector3 rad = eulerDegrees * (3.14159265f / 180.0f);
        rotation_ = Quaternion::CreateFromYawPitchRoll(rad.y, rad.x, rad.z);
        MarkDirty();
    }

    void SetEulerAngles(float pitch, float yaw, float roll) {
        SetEulerAngles(Vector3(pitch, yaw, roll));
    }

    //! @brief 軸周りに回転（度）
    void Rotate(const Vector3& axis, float angleDegrees) {
        float rad = angleDegrees * (3.14159265f / 180.0f);
        Quaternion delta = Quaternion::CreateFromAxisAngle(axis, rad);
        rotation_ = Quaternion::Concatenate(rotation_, delta);
        rotation_.Normalize();
        MarkDirty();
    }

    //! @brief オイラー角で回転を加算（度）
    void Rotate(const Vector3& eulerDegrees) {
        Vector3 rad = eulerDegrees * (3.14159265f / 180.0f);
        Quaternion delta = Quaternion::CreateFromYawPitchRoll(rad.y, rad.x, rad.z);
        rotation_ = Quaternion::Concatenate(rotation_, delta);
        rotation_.Normalize();
        MarkDirty();
    }

    void Rotate(float pitchDeg, float yawDeg, float rollDeg) {
        Rotate(Vector3(pitchDeg, yawDeg, rollDeg));
    }

    //! @brief ターゲットを向く
    void LookAt(const Vector3& target) {
        LookAt(target, Vector3::Up);
    }

    //! @brief ターゲットを向く（上方向指定）
    void LookAt(const Vector3& target, const Vector3& up) {
        Vector3 forward = target - position_;
        if (forward.LengthSquared() < 0.0001f) return;

        forward.Normalize();
        rotation_ = QuaternionLookRotation(forward, up);
        MarkDirty();
    }

    //========================================================================
    // スケール
    //========================================================================

    [[nodiscard]] const Vector3& GetScale() const noexcept { return scale_; }
    [[nodiscard]] const Vector3& GetLocalScale() const noexcept { return scale_; }

    void SetScale(const Vector3& scale) {
        scale_ = scale;
        MarkDirty();
    }

    void SetScale(float uniformScale) {
        SetScale(Vector3(uniformScale, uniformScale, uniformScale));
    }

    void SetScale(float x, float y, float z) {
        SetScale(Vector3(x, y, z));
    }

    //========================================================================
    // 方向ベクトル
    //========================================================================

    //! @brief 前方向ベクトル（+Z、左手座標系）
    [[nodiscard]] Vector3 GetForward() const {
        return Vector3::Transform(LH::Forward(), rotation_);
    }

    //! @brief 右方向ベクトル（+X）
    [[nodiscard]] Vector3 GetRight() const {
        return Vector3::Transform(Vector3::Right, rotation_);
    }

    //! @brief 上方向ベクトル（+Y）
    [[nodiscard]] Vector3 GetUp() const {
        return Vector3::Transform(Vector3::Up, rotation_);
    }

    //! @brief 後方向ベクトル（-Z）
    [[nodiscard]] Vector3 GetBack() const {
        return -GetForward();
    }

    //! @brief 左方向ベクトル（-X）
    [[nodiscard]] Vector3 GetLeft() const {
        return -GetRight();
    }

    //! @brief 下方向ベクトル（-Y）
    [[nodiscard]] Vector3 GetDown() const {
        return -GetUp();
    }

    //========================================================================
    // 行列
    //========================================================================

    //! @brief ワールド変換行列を取得
    [[nodiscard]] const Matrix& GetWorldMatrix() {
        if (isDirty_) {
            UpdateWorldMatrix();
        }
        return worldMatrix_;
    }

    //! @brief ローカル変換行列を取得
    [[nodiscard]] Matrix GetLocalMatrix() const {
        return Matrix::CreateScale(scale_) *
               Matrix::CreateFromQuaternion(rotation_) *
               Matrix::CreateTranslation(position_);
    }

    //========================================================================
    // 階層
    //========================================================================

    //! @brief 親Transformを取得
    [[nodiscard]] Transform* GetParent() const {
        if (auto* go = GetGameObject()) {
            if (auto* parentGO = go->GetParent()) {
                return parentGO->GetComponent<Transform>();
            }
        }
        return nullptr;
    }

    //! @brief ワールド座標での位置を取得
    [[nodiscard]] Vector3 GetWorldPosition() {
        if (auto* parent = GetParent()) {
            return Vector3::Transform(position_, parent->GetWorldMatrix());
        }
        return position_;
    }

    //! @brief ワールド座標での回転を取得
    [[nodiscard]] Quaternion GetWorldRotation() {
        if (auto* parent = GetParent()) {
            return Quaternion::Concatenate(rotation_, parent->GetWorldRotation());
        }
        return rotation_;
    }

    //========================================================================
    // ユーティリティ
    //========================================================================

    //! @brief 変換をリセット
    void Reset() {
        position_ = Vector3::Zero;
        rotation_ = Quaternion::Identity;
        scale_ = Vector3::One;
        MarkDirty();
    }

    //! @brief 点をローカル座標からワールド座標に変換
    [[nodiscard]] Vector3 TransformPoint(const Vector3& localPoint) {
        return Vector3::Transform(localPoint, GetWorldMatrix());
    }

    //! @brief 方向をローカルからワールドに変換
    [[nodiscard]] Vector3 TransformDirection(const Vector3& localDirection) const {
        return Vector3::TransformNormal(localDirection, Matrix::CreateFromQuaternion(rotation_));
    }

    //! @brief 点をワールド座標からローカル座標に変換
    [[nodiscard]] Vector3 InverseTransformPoint(const Vector3& worldPoint) {
        Matrix invWorld;
        GetWorldMatrix().Invert(invWorld);
        return Vector3::Transform(worldPoint, invWorld);
    }

    //! @brief 方向をワールドからローカルに変換
    [[nodiscard]] Vector3 InverseTransformDirection(const Vector3& worldDirection) const {
        Quaternion invRot;
        rotation_.Inverse(invRot);
        return Vector3::Transform(worldDirection, invRot);
    }

    //! @brief 変更フラグ
    [[nodiscard]] bool IsDirty() const noexcept { return isDirty_; }

    //! @brief コールバック: 変換が変更されたとき
    std::function<void()> OnTransformChanged;

private:
    void MarkDirty() {
        isDirty_ = true;
    }

    void UpdateWorldMatrix() {
        // ローカル行列を計算
        Matrix local = GetLocalMatrix();

        // 親がいれば親のワールド行列と合成
        if (auto* parent = GetParent()) {
            worldMatrix_ = local * parent->GetWorldMatrix();
        } else {
            worldMatrix_ = local;
        }

        isDirty_ = false;

        if (OnTransformChanged) {
            OnTransformChanged();
        }
    }

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------
    static Vector3 QuaternionToEuler(const Quaternion& q) {
        Vector3 euler;

        // Roll (X)
        float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
        float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
        euler.x = std::atan2(sinr_cosp, cosr_cosp);

        // Pitch (Y)
        float sinp = 2.0f * (q.w * q.y - q.z * q.x);
        if (std::abs(sinp) >= 1.0f) {
            euler.y = std::copysign(3.14159265f / 2.0f, sinp);
        } else {
            euler.y = std::asin(sinp);
        }

        // Yaw (Z)
        float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
        float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        euler.z = std::atan2(siny_cosp, cosy_cosp);

        // ラジアンから度へ
        return euler * (180.0f / 3.14159265f);
    }

    static Quaternion QuaternionLookRotation(const Vector3& forward, const Vector3& up) {
        Vector3 f = forward;
        f.Normalize();

        Vector3 r = up.Cross(f);
        r.Normalize();

        Vector3 u = f.Cross(r);

        Matrix m = Matrix::Identity;
        m._11 = r.x; m._12 = r.y; m._13 = r.z;
        m._21 = u.x; m._22 = u.y; m._23 = u.z;
        m._31 = f.x; m._32 = f.y; m._33 = f.z;

        return Quaternion::CreateFromRotationMatrix(m);
    }

    // データ
    Vector3 position_ = Vector3::Zero;
    Quaternion rotation_ = Quaternion::Identity;
    Vector3 scale_ = Vector3::One;
    Matrix worldMatrix_ = Matrix::Identity;
    bool isDirty_ = true;
};

OOP_COMPONENT(Transform);
