//----------------------------------------------------------------------------
//! @file   camera.h
//! @brief  Camera - 純粋OOPカメラコンポーネント
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component.h"
#include "engine/math/math_types.h"
#include "transform.h"
#include <functional>

//============================================================================
//! @brief カメラ投影モード
//============================================================================
enum class CameraProjection {
    Perspective,    //!< 透視投影（3D）
    Orthographic    //!< 正投影（2D）
};

//============================================================================
//! @brief カメラクリアモード
//============================================================================
enum class CameraClearFlags {
    Skybox,         //!< スカイボックスでクリア
    SolidColor,     //!< 単色でクリア
    DepthOnly,      //!< 深度のみクリア
    Nothing         //!< クリアしない
};

//============================================================================
//! @brief Camera - カメラコンポーネント
//!
//! Unity MonoBehaviour風の設計。カメラの設定と行列計算を管理。
//!
//! @code
//! auto* go = world.CreateGameObject("MainCamera");
//! go->AddComponent<Transform>(Vector3(0, 5, -10));
//! auto* cam = go->AddComponent<Camera>();
//!
//! cam->SetFieldOfView(60.0f);
//! cam->SetNearClip(0.1f);
//! cam->SetFarClip(1000.0f);
//! cam->LookAt(Vector3::Zero);
//!
//! Matrix view = cam->GetViewMatrix();
//! Matrix proj = cam->GetProjectionMatrix();
//! @endcode
//============================================================================
class Camera final : public Component {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Camera() = default;

    Camera(float fov, float aspectRatio)
        : fieldOfView_(fov)
        , aspectRatio_(aspectRatio) {}

    //------------------------------------------------------------------------
    // ライフサイクル
    //------------------------------------------------------------------------
    void Start() override {
        transform_ = GetComponent<Transform>();
        UpdateMatrices();
    }

    void LateUpdate(float dt) override {
        if (isDirty_ || (transform_ && transform_->IsDirty())) {
            UpdateMatrices();
        }
    }

    //========================================================================
    // 投影設定
    //========================================================================

    [[nodiscard]] CameraProjection GetProjection() const noexcept { return projection_; }

    void SetProjection(CameraProjection proj) {
        projection_ = proj;
        MarkDirty();
    }

    void SetPerspective(float fov, float aspectRatio, float nearClip, float farClip) {
        projection_ = CameraProjection::Perspective;
        fieldOfView_ = fov;
        aspectRatio_ = aspectRatio;
        nearClip_ = nearClip;
        farClip_ = farClip;
        MarkDirty();
    }

    void SetOrthographic(float size, float aspectRatio, float nearClip, float farClip) {
        projection_ = CameraProjection::Orthographic;
        orthographicSize_ = size;
        aspectRatio_ = aspectRatio;
        nearClip_ = nearClip;
        farClip_ = farClip;
        MarkDirty();
    }

    //========================================================================
    // 透視投影パラメータ
    //========================================================================

    [[nodiscard]] float GetFieldOfView() const noexcept { return fieldOfView_; }
    [[nodiscard]] float GetAspectRatio() const noexcept { return aspectRatio_; }
    [[nodiscard]] float GetNearClip() const noexcept { return nearClip_; }
    [[nodiscard]] float GetFarClip() const noexcept { return farClip_; }

    void SetFieldOfView(float fov) { fieldOfView_ = fov; MarkDirty(); }
    void SetAspectRatio(float ratio) { aspectRatio_ = ratio; MarkDirty(); }
    void SetNearClip(float near) { nearClip_ = near; MarkDirty(); }
    void SetFarClip(float far) { farClip_ = far; MarkDirty(); }

    //========================================================================
    // 正投影パラメータ
    //========================================================================

    [[nodiscard]] float GetOrthographicSize() const noexcept { return orthographicSize_; }
    void SetOrthographicSize(float size) { orthographicSize_ = size; MarkDirty(); }

    //========================================================================
    // クリア設定
    //========================================================================

    [[nodiscard]] CameraClearFlags GetClearFlags() const noexcept { return clearFlags_; }
    [[nodiscard]] const Color& GetBackgroundColor() const noexcept { return backgroundColor_; }

    void SetClearFlags(CameraClearFlags flags) noexcept { clearFlags_ = flags; }
    void SetBackgroundColor(const Color& color) noexcept { backgroundColor_ = color; }

    //========================================================================
    // 行列
    //========================================================================

    //! @brief ビュー行列を取得
    [[nodiscard]] const Matrix& GetViewMatrix() {
        if (isDirty_) UpdateMatrices();
        return viewMatrix_;
    }

    //! @brief 投影行列を取得
    [[nodiscard]] const Matrix& GetProjectionMatrix() {
        if (isDirty_) UpdateMatrices();
        return projectionMatrix_;
    }

    //! @brief ビュー投影行列を取得
    [[nodiscard]] Matrix GetViewProjectionMatrix() {
        return GetViewMatrix() * GetProjectionMatrix();
    }

    //========================================================================
    // ターゲット
    //========================================================================

    //! @brief ターゲットを向く
    void LookAt(const Vector3& target) {
        if (transform_) {
            transform_->LookAt(target);
        }
    }

    //! @brief ターゲットを向く（上方向指定）
    void LookAt(const Vector3& target, const Vector3& up) {
        if (transform_) {
            transform_->LookAt(target, up);
        }
    }

    //========================================================================
    // スクリーン座標変換
    //========================================================================

    //! @brief ワールド座標をスクリーン座標に変換
    //! @param worldPoint ワールド座標
    //! @param screenWidth スクリーン幅
    //! @param screenHeight スクリーン高さ
    //! @return スクリーン座標 (x, y, depth)
    [[nodiscard]] Vector3 WorldToScreenPoint(const Vector3& worldPoint,
                                              float screenWidth, float screenHeight) {
        Matrix vp = GetViewProjectionMatrix();
        Vector4 clipPos = Vector4::Transform(Vector4(worldPoint.x, worldPoint.y, worldPoint.z, 1.0f), vp);

        if (std::abs(clipPos.w) < 0.0001f) {
            return Vector3::Zero;
        }

        // NDC座標に変換
        Vector3 ndc(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);

        // スクリーン座標に変換
        return Vector3(
            (ndc.x + 1.0f) * 0.5f * screenWidth,
            (1.0f - ndc.y) * 0.5f * screenHeight,  // Y反転
            ndc.z
        );
    }

    //! @brief スクリーン座標をワールド座標に変換
    //! @param screenPoint スクリーン座標 (x, y, depth)
    //! @param screenWidth スクリーン幅
    //! @param screenHeight スクリーン高さ
    //! @return ワールド座標
    [[nodiscard]] Vector3 ScreenToWorldPoint(const Vector3& screenPoint,
                                              float screenWidth, float screenHeight) {
        // スクリーン座標をNDCに変換
        Vector3 ndc(
            (screenPoint.x / screenWidth) * 2.0f - 1.0f,
            1.0f - (screenPoint.y / screenHeight) * 2.0f,  // Y反転
            screenPoint.z
        );

        // 逆行列で変換
        Matrix invVP;
        GetViewProjectionMatrix().Invert(invVP);

        Vector4 worldPos = Vector4::Transform(Vector4(ndc.x, ndc.y, ndc.z, 1.0f), invVP);

        if (std::abs(worldPos.w) < 0.0001f) {
            return Vector3::Zero;
        }

        return Vector3(worldPos.x / worldPos.w, worldPos.y / worldPos.w, worldPos.z / worldPos.w);
    }

    //! @brief スクリーン座標からレイを生成
    //! @param screenPoint スクリーン座標 (x, y)
    //! @param screenWidth スクリーン幅
    //! @param screenHeight スクリーン高さ
    //! @param origin [out] レイの原点
    //! @param direction [out] レイの方向
    void ScreenPointToRay(const Vector2& screenPoint, float screenWidth, float screenHeight,
                          Vector3& origin, Vector3& direction) {
        Vector3 nearPoint = ScreenToWorldPoint(Vector3(screenPoint.x, screenPoint.y, 0.0f),
                                                screenWidth, screenHeight);
        Vector3 farPoint = ScreenToWorldPoint(Vector3(screenPoint.x, screenPoint.y, 1.0f),
                                               screenWidth, screenHeight);

        origin = nearPoint;
        direction = farPoint - nearPoint;
        direction.Normalize();
    }

    //========================================================================
    // ビューポート
    //========================================================================

    //! @brief ビューポート矩形を取得 (x, y, width, height) 正規化座標 [0-1]
    [[nodiscard]] const Vector4& GetViewportRect() const noexcept { return viewportRect_; }

    void SetViewportRect(const Vector4& rect) noexcept { viewportRect_ = rect; }
    void SetViewportRect(float x, float y, float width, float height) noexcept {
        viewportRect_ = Vector4(x, y, width, height);
    }

    //========================================================================
    // レイヤー・深度
    //========================================================================

    [[nodiscard]] uint32_t GetCullingMask() const noexcept { return cullingMask_; }
    [[nodiscard]] float GetDepth() const noexcept { return depth_; }

    void SetCullingMask(uint32_t mask) noexcept { cullingMask_ = mask; }
    void SetDepth(float depth) noexcept { depth_ = depth; }

    //========================================================================
    // フラスタム
    //========================================================================

    //! @brief 点がフラスタム内にあるか
    [[nodiscard]] bool IsPointInFrustum(const Vector3& point) {
        Vector3 screenPoint = WorldToScreenPoint(point, 1.0f, 1.0f);
        return screenPoint.x >= 0.0f && screenPoint.x <= 1.0f &&
               screenPoint.y >= 0.0f && screenPoint.y <= 1.0f &&
               screenPoint.z >= 0.0f && screenPoint.z <= 1.0f;
    }

    //! @brief AABBがフラスタム内にあるか（簡易判定）
    [[nodiscard]] bool IsBoundsInFrustum(const Vector3& min, const Vector3& max) {
        // 8頂点のいずれかがフラスタム内にあればtrue
        Vector3 corners[8] = {
            Vector3(min.x, min.y, min.z),
            Vector3(max.x, min.y, min.z),
            Vector3(min.x, max.y, min.z),
            Vector3(max.x, max.y, min.z),
            Vector3(min.x, min.y, max.z),
            Vector3(max.x, min.y, max.z),
            Vector3(min.x, max.y, max.z),
            Vector3(max.x, max.y, max.z)
        };

        for (int i = 0; i < 8; ++i) {
            if (IsPointInFrustum(corners[i])) {
                return true;
            }
        }
        return false;
    }

private:
    void MarkDirty() { isDirty_ = true; }

    void UpdateMatrices() {
        // ビュー行列（左手座標系）
        if (transform_) {
            Vector3 position = transform_->GetPosition();
            Vector3 forward = transform_->GetForward();
            Vector3 up = transform_->GetUp();
            viewMatrix_ = LH::CreateLookAt(position, position + forward, up);
        } else {
            viewMatrix_ = Matrix::Identity;
        }

        // 投影行列（左手座標系）
        if (projection_ == CameraProjection::Perspective) {
            float fovRad = fieldOfView_ * (3.14159265f / 180.0f);
            projectionMatrix_ = LH::CreatePerspectiveFov(
                fovRad, aspectRatio_, nearClip_, farClip_);
        } else {
            float width = orthographicSize_ * aspectRatio_;
            float height = orthographicSize_;
            projectionMatrix_ = LH::CreateOrthographic(
                width, height, nearClip_, farClip_);
        }

        isDirty_ = false;
    }

    Transform* transform_ = nullptr;

    // 投影設定
    CameraProjection projection_ = CameraProjection::Perspective;
    float fieldOfView_ = 60.0f;        // 度
    float aspectRatio_ = 16.0f / 9.0f;
    float nearClip_ = 0.1f;
    float farClip_ = 1000.0f;
    float orthographicSize_ = 5.0f;    // 正投影時の高さの半分

    // クリア設定
    CameraClearFlags clearFlags_ = CameraClearFlags::SolidColor;
    Color backgroundColor_ = Color(0.2f, 0.2f, 0.3f, 1.0f);

    // ビューポート
    Vector4 viewportRect_ = Vector4(0, 0, 1, 1);  // 全画面

    // レイヤー・深度
    uint32_t cullingMask_ = 0xFFFFFFFF;
    float depth_ = 0.0f;

    // 行列キャッシュ
    Matrix viewMatrix_ = Matrix::Identity;
    Matrix projectionMatrix_ = Matrix::Identity;
    bool isDirty_ = true;
};

OOP_COMPONENT(Camera);
