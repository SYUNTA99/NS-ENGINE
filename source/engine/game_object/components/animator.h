//----------------------------------------------------------------------------
//! @file   animator.h
//! @brief  Animator - Unity風3Dアニメーションコンポーネント
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component.h"
#include "engine/math/math_types.h"
#include "transform.h"
#include "animation/skeleton.h"
#include "animation/animation_clip.h"
#include "animation/animator_controller.h"
#include "animation/animator_state_info.h"
#include <functional>
#include <vector>
#include <memory>

//============================================================================
//! @brief レイヤー再生状態
//============================================================================
struct LayerPlaybackState {
    int currentStateIndex = 0;          //!< 現在のステートインデックス
    float normalizedTime = 0.0f;        //!< 正規化時間

    // ブレンド状態
    bool isBlending = false;            //!< ブレンド中か
    int previousStateIndex = -1;        //!< 前のステートインデックス
    float previousNormalizedTime = 0.0f;//!< 前のステートの正規化時間
    float blendWeight = 1.0f;           //!< 現在ステートのウェイト（1.0=完全に現在ステート）
    float blendDuration = 0.0f;         //!< ブレンド時間
    float blendElapsed = 0.0f;          //!< ブレンド経過時間

    LayerPlaybackState() = default;

    //! @brief ブレンドを開始
    void StartBlend(int newStateIndex, float duration) {
        if (currentStateIndex == newStateIndex) return;

        previousStateIndex = currentStateIndex;
        previousNormalizedTime = normalizedTime;
        currentStateIndex = newStateIndex;
        normalizedTime = 0.0f;

        isBlending = true;
        blendWeight = 0.0f;
        blendDuration = duration;
        blendElapsed = 0.0f;
    }

    //! @brief 即座に切り替え
    void SwitchImmediate(int newStateIndex, float startTime = 0.0f) {
        currentStateIndex = newStateIndex;
        normalizedTime = startTime;
        isBlending = false;
        previousStateIndex = -1;
        blendWeight = 1.0f;
    }

    //! @brief ブレンドを更新
    void UpdateBlend(float dt) {
        if (!isBlending) return;

        blendElapsed += dt;
        if (blendElapsed >= blendDuration) {
            // ブレンド完了
            isBlending = false;
            blendWeight = 1.0f;
            previousStateIndex = -1;
        } else {
            // ブレンド中
            blendWeight = blendElapsed / blendDuration;
        }
    }
};

//============================================================================
//! @brief Animator - Unity風3Dアニメーションコンポーネント
//!
//! スケルタルアニメーションを制御するメインコンポーネント。
//! AnimatorControllerによるステートマシン、クロスフェード、
//! アニメーションイベントなどUnity Animatorと同等の機能を提供。
//!
//! @code
//! // GameObjectにAnimatorを追加
//! auto* go = world.CreateGameObject("Character");
//! go->AddComponent<Transform>();
//! go->AddComponent<MeshRenderer>(skinnedMesh);
//! auto* anim = go->AddComponent<Animator>();
//!
//! // コントローラーとスケルトン設定
//! anim->SetController(characterController);
//! anim->SetSkeleton(characterSkeleton);
//!
//! // パラメータ制御
//! anim->SetFloat("Speed", 5.0f);
//! anim->SetBool("IsGrounded", true);
//! anim->SetTrigger("Jump");
//!
//! // 直接再生
//! anim->Play("Attack", 0, 0.0f);
//!
//! // クロスフェード
//! anim->CrossFade("Walk", 0.2f);
//!
//! // 状態情報取得
//! AnimatorStateInfo info = anim->GetCurrentAnimatorStateInfo(0);
//! if (info.IsName("Idle") && info.normalizedTime > 0.9f) {
//!     // Idle終了間近
//! }
//!
//! // イベントコールバック
//! anim->OnAnimationEvent = [](const std::string& name, const AnimationEvent& e) {
//!     if (name == "FootStep") {
//!         PlayFootstepSound();
//!     }
//! };
//! @endcode
//============================================================================
class Animator final : public Component {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    Animator() = default;

    explicit Animator(AnimatorControllerPtr controller)
        : controller_(std::move(controller)) {}

    Animator(AnimatorControllerPtr controller, SkeletonPtr skeleton)
        : controller_(std::move(controller))
        , skeleton_(std::move(skeleton)) {}

    //------------------------------------------------------------------------
    // ライフサイクル
    //------------------------------------------------------------------------
    void Start() override {
        transform_ = GetComponent<Transform>();
        InitializeFromController();
    }

    void Update(float dt) override {
        if (!controller_ || !skeleton_) return;
        if (updateInFixedTime_) return;  // FixedUpdateで更新する場合はスキップ

        UpdateAnimation(dt * speed_);
    }

    void FixedUpdate(float dt) override {
        if (!controller_ || !skeleton_) return;
        if (!updateInFixedTime_) return;  // Updateで更新する場合はスキップ

        UpdateAnimation(dt * speed_);
    }

    void LateUpdate([[maybe_unused]] float dt) override {
        // ルートモーションの適用
        if (applyRootMotion_ && transform_) {
            transform_->Translate(deltaPosition_);
            Quaternion currentRot = transform_->GetRotation();
            transform_->SetRotation(currentRot * deltaRotation_);
        }

        // 次フレーム用にリセット
        deltaPosition_ = Vector3::Zero;
        deltaRotation_ = Quaternion::Identity;
    }

    //========================================================================
    // セットアップ
    //========================================================================

    //! @brief コントローラーを設定
    void SetController(AnimatorControllerPtr controller) {
        controller_ = std::move(controller);
        InitializeFromController();
    }

    //! @brief スケルトンを設定
    void SetSkeleton(SkeletonPtr skeleton) {
        skeleton_ = std::move(skeleton);
        if (skeleton_) {
            size_t boneCount = skeleton_->GetBoneCount();
            localBoneTransforms_.resize(boneCount, Matrix::Identity);
            globalBoneTransforms_.resize(boneCount, Matrix::Identity);
            skinningMatrices_.resize(boneCount, Matrix::Identity);
        }
    }

    [[nodiscard]] AnimatorController* GetController() const { return controller_.get(); }
    [[nodiscard]] Skeleton* GetSkeleton() const { return skeleton_.get(); }

    //========================================================================
    // パラメータ制御
    //========================================================================

    void SetFloat(const std::string& name, float value) {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            it->second.SetFloat(value);
        }
    }

    void SetInt(const std::string& name, int value) {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            it->second.SetInt(value);
        }
    }

    void SetBool(const std::string& name, bool value) {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            it->second.SetBool(value);
        }
    }

    void SetTrigger(const std::string& name) {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            it->second.SetTrigger();
        }
    }

    void ResetTrigger(const std::string& name) {
        auto it = parameters_.find(name);
        if (it != parameters_.end()) {
            it->second.ResetTrigger();
        }
    }

    [[nodiscard]] float GetFloat(const std::string& name) const {
        auto it = parameters_.find(name);
        return (it != parameters_.end()) ? it->second.GetFloat() : 0.0f;
    }

    [[nodiscard]] int GetInt(const std::string& name) const {
        auto it = parameters_.find(name);
        return (it != parameters_.end()) ? it->second.GetInt() : 0;
    }

    [[nodiscard]] bool GetBool(const std::string& name) const {
        auto it = parameters_.find(name);
        return (it != parameters_.end()) ? it->second.GetBool() : false;
    }

    //========================================================================
    // 再生制御
    //========================================================================

    //! @brief ステートを直接再生
    //! @param stateName ステート名
    //! @param layer レイヤーインデックス
    //! @param normalizedTime 開始位置（正規化時間）
    void Play(const std::string& stateName, int layer = 0, float normalizedTime = 0.0f) {
        if (!controller_ || layer < 0 || layer >= static_cast<int>(layerStates_.size())) {
            return;
        }

        auto* layerDef = controller_->GetLayer(layer);
        if (!layerDef) return;

        int stateIndex = layerDef->FindStateIndex(stateName);
        if (stateIndex >= 0) {
            layerStates_[layer].SwitchImmediate(stateIndex, normalizedTime);
        }
    }

    //! @brief クロスフェード再生
    //! @param stateName ステート名
    //! @param duration ブレンド時間（秒）
    //! @param layer レイヤーインデックス
    void CrossFade(const std::string& stateName, float duration, int layer = 0) {
        if (!controller_ || layer < 0 || layer >= static_cast<int>(layerStates_.size())) {
            return;
        }

        auto* layerDef = controller_->GetLayer(layer);
        if (!layerDef) return;

        int stateIndex = layerDef->FindStateIndex(stateName);
        if (stateIndex >= 0) {
            layerStates_[layer].StartBlend(stateIndex, duration);
        }
    }

    //! @brief クロスフェード再生（正規化時間指定）
    void CrossFadeInFixedTime(const std::string& stateName, float fixedDuration, int layer = 0) {
        CrossFade(stateName, fixedDuration, layer);
    }

    //========================================================================
    // 状態情報
    //========================================================================

    //! @brief 現在の状態情報を取得
    [[nodiscard]] AnimatorStateInfo GetCurrentAnimatorStateInfo(int layer = 0) const {
        AnimatorStateInfo info;

        if (!controller_ || layer < 0 || layer >= static_cast<int>(layerStates_.size())) {
            return info;
        }

        auto* layerDef = controller_->GetLayer(layer);
        if (!layerDef) return info;

        const auto& playback = layerStates_[layer];
        auto* state = layerDef->GetState(playback.currentStateIndex);
        if (!state) return info;

        info.stateName = state->name;
        info.stateNameHash = state->GetNameHash();
        info.tag = state->tag;
        info.normalizedTime = playback.normalizedTime;
        info.length = state->GetLength();
        info.speed = state->speed;
        info.loop = state->loop;
        info.layerIndex = layer;
        info.stateIndex = playback.currentStateIndex;

        return info;
    }

    //! @brief 次の状態情報を取得（遷移中）
    [[nodiscard]] AnimatorStateInfo GetNextAnimatorStateInfo(int layer = 0) const {
        AnimatorStateInfo info;

        if (!controller_ || layer < 0 || layer >= static_cast<int>(layerStates_.size())) {
            return info;
        }

        const auto& playback = layerStates_[layer];
        if (!playback.isBlending) {
            return info;  // 遷移中でない
        }

        // 次のステートは現在のステート
        return GetCurrentAnimatorStateInfo(layer);
    }

    //! @brief 遷移中か
    [[nodiscard]] bool IsInTransition(int layer = 0) const {
        if (layer < 0 || layer >= static_cast<int>(layerStates_.size())) {
            return false;
        }
        return layerStates_[layer].isBlending;
    }

    //! @brief 遷移情報を取得
    [[nodiscard]] AnimatorTransitionInfo GetAnimatorTransitionInfo(int layer = 0) const {
        AnimatorTransitionInfo info;

        if (!controller_ || layer < 0 || layer >= static_cast<int>(layerStates_.size())) {
            return info;
        }

        auto* layerDef = controller_->GetLayer(layer);
        if (!layerDef) return info;

        const auto& playback = layerStates_[layer];
        if (!playback.isBlending) return info;

        auto* prevState = layerDef->GetState(playback.previousStateIndex);
        auto* currState = layerDef->GetState(playback.currentStateIndex);

        if (prevState) info.sourceStateName = prevState->name;
        if (currState) info.destinationStateName = currState->name;
        info.normalizedTime = playback.blendWeight;
        info.duration = playback.blendDuration;
        info.sourceStateIndex = playback.previousStateIndex;
        info.destinationStateIndex = playback.currentStateIndex;

        return info;
    }

    //========================================================================
    // ボーンアクセス
    //========================================================================

    //! @brief ボーンのローカルTransformを取得
    [[nodiscard]] Matrix GetBoneLocalTransform(int boneIndex) const {
        if (boneIndex >= 0 && boneIndex < static_cast<int>(localBoneTransforms_.size())) {
            return localBoneTransforms_[boneIndex];
        }
        return Matrix::Identity;
    }

    //! @brief ボーンのグローバルTransformを取得
    [[nodiscard]] Matrix GetBoneGlobalTransform(int boneIndex) const {
        if (boneIndex >= 0 && boneIndex < static_cast<int>(globalBoneTransforms_.size())) {
            return globalBoneTransforms_[boneIndex];
        }
        return Matrix::Identity;
    }

    //! @brief ボーンのローカルTransformを設定（IK用）
    void SetBoneLocalTransform(int boneIndex, const Matrix& transform) {
        if (boneIndex >= 0 && boneIndex < static_cast<int>(localBoneTransforms_.size())) {
            localBoneTransforms_[boneIndex] = transform;
        }
    }

    //========================================================================
    // スキニング行列（レンダラー用）
    //========================================================================

    //! @brief 最終スキニング行列を取得（GPU送信用）
    [[nodiscard]] const std::vector<Matrix>& GetSkinningMatrices() const {
        return skinningMatrices_;
    }

    //! @brief ボーン数を取得
    [[nodiscard]] size_t GetBoneCount() const {
        return skeleton_ ? skeleton_->GetBoneCount() : 0;
    }

    //========================================================================
    // イベント
    //========================================================================

    //! @brief アニメーションイベントコールバック
    std::function<void(const std::string& eventName, const AnimationEvent& event)> OnAnimationEvent;

    //========================================================================
    // 速度・更新モード
    //========================================================================

    [[nodiscard]] float GetSpeed() const noexcept { return speed_; }
    void SetSpeed(float speed) noexcept { speed_ = speed; }

    [[nodiscard]] bool GetUpdateMode() const noexcept { return updateInFixedTime_; }
    void SetUpdateMode(bool fixedTime) noexcept { updateInFixedTime_ = fixedTime; }

    //========================================================================
    // ルートモーション
    //========================================================================

    [[nodiscard]] bool GetApplyRootMotion() const noexcept { return applyRootMotion_; }
    void SetApplyRootMotion(bool apply) noexcept { applyRootMotion_ = apply; }

    [[nodiscard]] Vector3 GetDeltaPosition() const noexcept { return deltaPosition_; }
    [[nodiscard]] Quaternion GetDeltaRotation() const noexcept { return deltaRotation_; }

    //========================================================================
    // レイヤー情報
    //========================================================================

    [[nodiscard]] int GetLayerCount() const {
        return static_cast<int>(layerStates_.size());
    }

    [[nodiscard]] float GetLayerWeight(int layer) const {
        if (!controller_) return 0.0f;
        auto* layerDef = controller_->GetLayer(layer);
        return layerDef ? layerDef->weight : 0.0f;
    }

    void SetLayerWeight(int layer, float weight) {
        if (!controller_) return;
        auto* layerDef = controller_->GetLayer(layer);
        if (layerDef) {
            layerDef->weight = weight;
        }
    }

private:
    //------------------------------------------------------------------------
    // 初期化
    //------------------------------------------------------------------------
    void InitializeFromController() {
        if (!controller_) return;

        // パラメータをコピー
        parameters_ = controller_->CloneParameters();

        // レイヤー状態を初期化
        layerStates_.clear();
        layerStates_.resize(controller_->GetLayerCount());

        for (size_t i = 0; i < layerStates_.size(); ++i) {
            auto* layer = controller_->GetLayer(static_cast<int>(i));
            if (layer) {
                layerStates_[i].currentStateIndex = layer->defaultStateIndex;
            }
        }
    }

    //------------------------------------------------------------------------
    // アニメーション更新
    //------------------------------------------------------------------------
    void UpdateAnimation(float dt) {
        // 各レイヤーのステートマシンを更新
        for (size_t i = 0; i < layerStates_.size(); ++i) {
            UpdateLayer(static_cast<int>(i), dt);
        }

        // ボーンポーズを計算
        ComputeFinalPose();
    }

    void UpdateLayer(int layerIndex, float dt) {
        auto* layerDef = controller_->GetLayer(layerIndex);
        if (!layerDef) return;

        auto& playback = layerStates_[layerIndex];

        // 遷移評価
        EvaluateTransitions(layerIndex);

        // ブレンド更新
        playback.UpdateBlend(dt);

        // 時間更新
        auto* currentState = layerDef->GetState(playback.currentStateIndex);
        if (currentState && currentState->clip) {
            float prevTime = playback.normalizedTime;
            float clipDuration = currentState->clip->duration;

            if (clipDuration > 0.0f) {
                playback.normalizedTime += (dt * currentState->speed) / clipDuration;

                // ループ処理
                if (currentState->loop) {
                    while (playback.normalizedTime >= 1.0f) {
                        playback.normalizedTime -= 1.0f;
                    }
                } else {
                    if (playback.normalizedTime > 1.0f) {
                        playback.normalizedTime = 1.0f;
                    }
                }

                // イベント処理
                ProcessAnimationEvents(prevTime, playback.normalizedTime, *currentState->clip);
            }
        }

        // ブレンド中の前ステートも更新
        if (playback.isBlending && playback.previousStateIndex >= 0) {
            auto* prevState = layerDef->GetState(playback.previousStateIndex);
            if (prevState && prevState->clip && prevState->clip->duration > 0.0f) {
                playback.previousNormalizedTime += (dt * prevState->speed) / prevState->clip->duration;
                if (prevState->loop) {
                    while (playback.previousNormalizedTime >= 1.0f) {
                        playback.previousNormalizedTime -= 1.0f;
                    }
                }
            }
        }
    }

    void EvaluateTransitions(int layerIndex) {
        auto* layerDef = controller_->GetLayer(layerIndex);
        if (!layerDef) return;

        auto& playback = layerStates_[layerIndex];
        auto* currentState = layerDef->GetState(playback.currentStateIndex);
        if (!currentState) return;

        // Any State遷移を評価
        const auto& anyTransitions = controller_->GetAnyStateTransitions();
        for (size_t i = 0; i < anyTransitions.size(); ++i) {
            if (controller_->GetAnyStateTransitionLayer(i) != layerIndex) continue;

            const auto& transition = anyTransitions[i];
            if (transition.destinationStateIndex == playback.currentStateIndex &&
                !transition.canTransitionToSelf) {
                continue;  // 自身への遷移は許可されていない
            }

            if (transition.EvaluateConditions(parameters_, playback.normalizedTime)) {
                transition.ConsumeTriggers(parameters_);
                playback.StartBlend(transition.destinationStateIndex, transition.duration);
                return;
            }
        }

        // ステートの遷移を評価
        for (const auto& transition : currentState->transitions) {
            if (transition.destinationStateIndex == playback.currentStateIndex &&
                !transition.canTransitionToSelf) {
                continue;
            }

            if (transition.EvaluateConditions(parameters_, playback.normalizedTime)) {
                transition.ConsumeTriggers(parameters_);
                playback.StartBlend(transition.destinationStateIndex, transition.duration);
                return;
            }
        }
    }

    void ProcessAnimationEvents(float prevTime, float currTime, const AnimationClip& clip) {
        if (!OnAnimationEvent) return;

        std::vector<const AnimationEvent*> events;
        clip.GetEventsInRange(
            prevTime * clip.duration,
            currTime * clip.duration,
            events);

        for (const auto* event : events) {
            OnAnimationEvent(event->functionName, *event);
        }
    }

    void ComputeFinalPose() {
        if (!skeleton_ || localBoneTransforms_.empty()) return;

        // 各レイヤーのポーズを合成
        std::fill(localBoneTransforms_.begin(), localBoneTransforms_.end(), Matrix::Identity);

        for (size_t layerIdx = 0; layerIdx < layerStates_.size(); ++layerIdx) {
            auto* layerDef = controller_->GetLayer(static_cast<int>(layerIdx));
            if (!layerDef) continue;

            const auto& playback = layerStates_[layerIdx];
            float layerWeight = layerDef->weight;

            if (layerWeight <= 0.0f) continue;

            // 現在ステートをサンプリング
            auto* currentState = layerDef->GetState(playback.currentStateIndex);
            if (currentState && currentState->clip) {
                std::vector<Matrix> currentPose(skeleton_->GetBoneCount(), Matrix::Identity);
                currentState->clip->SamplePose(
                    playback.normalizedTime * currentState->clip->duration,
                    currentPose);

                // ブレンド中の場合
                if (playback.isBlending && playback.previousStateIndex >= 0) {
                    auto* prevState = layerDef->GetState(playback.previousStateIndex);
                    if (prevState && prevState->clip) {
                        std::vector<Matrix> prevPose(skeleton_->GetBoneCount(), Matrix::Identity);
                        prevState->clip->SamplePose(
                            playback.previousNormalizedTime * prevState->clip->duration,
                            prevPose);

                        // ポーズをブレンド
                        for (size_t i = 0; i < currentPose.size(); ++i) {
                            currentPose[i] = BlendMatrix(prevPose[i], currentPose[i], playback.blendWeight);
                        }
                    }
                }

                // レイヤーブレンド（最初のレイヤーは上書き、以降はブレンド）
                if (layerIdx == 0 || layerDef->blendingMode == LayerBlendingMode::Override) {
                    for (size_t i = 0; i < currentPose.size(); ++i) {
                        if (layerWeight >= 1.0f) {
                            localBoneTransforms_[i] = currentPose[i];
                        } else {
                            localBoneTransforms_[i] = BlendMatrix(localBoneTransforms_[i], currentPose[i], layerWeight);
                        }
                    }
                }
                // Additiveモードは将来実装
            }
        }

        // グローバル変換を計算
        skeleton_->ComputeGlobalTransforms(localBoneTransforms_, globalBoneTransforms_);

        // スキニング行列を計算
        skeleton_->ComputeSkinningMatrices(globalBoneTransforms_, skinningMatrices_);
    }

    //! @brief 行列のブレンド（TRS分解 + 回転はSlerp）
    //!
    //! 行列を Translation, Rotation, Scale に分解し、
    //! T/S は線形補間、R は球面線形補間（Slerp）でブレンドする。
    static Matrix BlendMatrix(const Matrix& a, const Matrix& b, float t) {
        // 行列をTRS分解
        Vector3 scaleA, scaleB;
        Quaternion rotA, rotB;
        Vector3 transA, transB;

        // 行列からTRSを抽出
        DecomposeMatrix(a, transA, rotA, scaleA);
        DecomposeMatrix(b, transB, rotB, scaleB);

        // Translation: 線形補間
        Vector3 trans = Vector3::Lerp(transA, transB, t);

        // Rotation: 球面線形補間（Slerp）
        Quaternion rot = Quaternion::Slerp(rotA, rotB, t);

        // Scale: 線形補間
        Vector3 scale = Vector3::Lerp(scaleA, scaleB, t);

        // TRSから行列を再構築
        return Matrix::CreateScale(scale) *
               Matrix::CreateFromQuaternion(rot) *
               Matrix::CreateTranslation(trans);
    }

    //! @brief 行列をTranslation, Rotation, Scaleに分解
    static void DecomposeMatrix(const Matrix& m, Vector3& translation, Quaternion& rotation, Vector3& scale) {
        // Translation: 行列の最後の列
        translation = Vector3(m._41, m._42, m._43);

        // Scale: 各軸ベクトルの長さ
        Vector3 axisX(m._11, m._12, m._13);
        Vector3 axisY(m._21, m._22, m._23);
        Vector3 axisZ(m._31, m._32, m._33);

        scale.x = axisX.Length();
        scale.y = axisY.Length();
        scale.z = axisZ.Length();

        // 回転行列を抽出（スケールを除去）
        if (scale.x > 0.0001f) axisX = axisX / scale.x;
        if (scale.y > 0.0001f) axisY = axisY / scale.y;
        if (scale.z > 0.0001f) axisZ = axisZ / scale.z;

        // 回転行列からクォータニオンへ
        Matrix rotMatrix(
            axisX.x, axisX.y, axisX.z, 0,
            axisY.x, axisY.y, axisY.z, 0,
            axisZ.x, axisZ.y, axisZ.z, 0,
            0, 0, 0, 1
        );
        rotation = Quaternion::CreateFromRotationMatrix(rotMatrix);
    }

    //------------------------------------------------------------------------
    // メンバ
    //------------------------------------------------------------------------
    AnimatorControllerPtr controller_;
    SkeletonPtr skeleton_;
    Transform* transform_ = nullptr;

    // 実行時パラメータ（コントローラーからコピー）
    std::unordered_map<std::string, AnimatorParameter> parameters_;

    // レイヤーごとの再生状態
    std::vector<LayerPlaybackState> layerStates_;

    // ボーンTransform
    std::vector<Matrix> localBoneTransforms_;
    std::vector<Matrix> globalBoneTransforms_;
    std::vector<Matrix> skinningMatrices_;

    // 再生設定
    float speed_ = 1.0f;
    bool updateInFixedTime_ = false;

    // ルートモーション
    bool applyRootMotion_ = false;
    Vector3 deltaPosition_ = Vector3::Zero;
    Quaternion deltaRotation_ = Quaternion::Identity;
};

OOP_COMPONENT(Animator);
