//----------------------------------------------------------------------------
//! @file   animator_controller.h
//! @brief  AnimatorController - ステートマシンコントローラー
//----------------------------------------------------------------------------
#pragma once


#include "animator_state.h"
#include "animator_parameter.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

//============================================================================
//! @brief レイヤーブレンドモード
//============================================================================
enum class LayerBlendingMode {
    Override,       //!< 上書き
    Additive        //!< 加算
};

//============================================================================
//! @brief AnimatorLayer - アニメーションレイヤー
//!
//! 1つのレイヤーは独立したステートマシンを持つ。
//! 複数レイヤーでボディアニメーションと上半身アニメーションを分けるなど。
//============================================================================
struct AnimatorLayer {
    std::string name;                                   //!< レイヤー名
    std::vector<AnimatorState> states;                  //!< ステート配列
    int defaultStateIndex = 0;                          //!< デフォルトステート
    float weight = 1.0f;                                //!< ブレンドウェイト
    LayerBlendingMode blendingMode = LayerBlendingMode::Override;  //!< ブレンドモード

    // アバターマスク（将来用）
    // uint32_t avatarMask = 0xFFFFFFFF;                //!< 適用するボーンマスク

    AnimatorLayer() = default;

    AnimatorLayer(const std::string& layerName)
        : name(layerName) {}

    //! @brief ステートを追加
    //! @return 追加されたステートのインデックス
    int AddState(const AnimatorState& state) {
        int index = static_cast<int>(states.size());
        states.push_back(state);
        return index;
    }

    //! @brief ステートを追加（名前とクリップ）
    int AddState(const std::string& name, AnimationClipPtr clip = nullptr) {
        return AddState(AnimatorState(name, std::move(clip)));
    }

    //! @brief 名前からステートインデックスを検索
    [[nodiscard]] int FindStateIndex(const std::string& stateName) const {
        for (size_t i = 0; i < states.size(); ++i) {
            if (states[i].name == stateName) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    //! @brief ステートを取得
    [[nodiscard]] AnimatorState* GetState(int index) {
        if (index >= 0 && index < static_cast<int>(states.size())) {
            return &states[index];
        }
        return nullptr;
    }

    [[nodiscard]] const AnimatorState* GetState(int index) const {
        if (index >= 0 && index < static_cast<int>(states.size())) {
            return &states[index];
        }
        return nullptr;
    }
};

//============================================================================
//! @brief AnimatorController - アニメーターコントローラー
//!
//! アニメーションステートマシンの定義を保持。
//! AnimatorコンポーネントがControllerを参照して状態を更新する。
//!
//! @code
//! auto controller = std::make_shared<AnimatorController>();
//! controller->name = "CharacterController";
//!
//! // パラメータ追加
//! controller->AddParameter("Speed", 0.0f);
//! controller->AddParameter("IsGrounded", true);
//! controller->AddTrigger("Jump");
//!
//! // レイヤー追加
//! auto& baseLayer = controller->AddLayer("Base Layer");
//!
//! // ステート追加
//! int idleIdx = baseLayer.AddState("Idle", idleClip);
//! int walkIdx = baseLayer.AddState("Walk", walkClip);
//! int runIdx = baseLayer.AddState("Run", runClip);
//! int jumpIdx = baseLayer.AddState("Jump", jumpClip);
//!
//! // 遷移追加
//! baseLayer.states[idleIdx].AddTransition(walkIdx, 0.2f)
//!     .AddCondition(TransitionCondition::FloatGreater("Speed", 0.1f));
//!
//! baseLayer.states[walkIdx].AddTransition(runIdx, 0.2f)
//!     .AddCondition(TransitionCondition::FloatGreater("Speed", 0.5f));
//!
//! baseLayer.states[walkIdx].AddTransition(idleIdx, 0.2f)
//!     .AddCondition(TransitionCondition::FloatLess("Speed", 0.1f));
//!
//! // Any State → Jump（どのステートからでもジャンプ可能）
//! controller->AddAnyStateTransition(0, jumpIdx, 0.1f)
//!     .AddCondition(TransitionCondition::Trigger("Jump"));
//! @endcode
//============================================================================
class AnimatorController {
public:
    std::string name;                                                   //!< コントローラー名

    //========================================================================
    // レイヤー管理
    //========================================================================

    //! @brief レイヤーを追加
    //! @return 追加されたレイヤーへの参照
    AnimatorLayer& AddLayer(const std::string& layerName = "Base Layer") {
        layers_.emplace_back(layerName);
        return layers_.back();
    }

    //! @brief レイヤー数を取得
    [[nodiscard]] size_t GetLayerCount() const noexcept {
        return layers_.size();
    }

    //! @brief レイヤーを取得
    [[nodiscard]] AnimatorLayer* GetLayer(int index) {
        if (index >= 0 && index < static_cast<int>(layers_.size())) {
            return &layers_[index];
        }
        return nullptr;
    }

    [[nodiscard]] const AnimatorLayer* GetLayer(int index) const {
        if (index >= 0 && index < static_cast<int>(layers_.size())) {
            return &layers_[index];
        }
        return nullptr;
    }

    //! @brief 名前からレイヤーを検索
    [[nodiscard]] int FindLayerIndex(const std::string& layerName) const {
        for (size_t i = 0; i < layers_.size(); ++i) {
            if (layers_[i].name == layerName) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    //========================================================================
    // パラメータ管理
    //========================================================================

    //! @brief Floatパラメータを追加
    void AddParameter(const std::string& name, float defaultValue) {
        parameters_[name] = AnimatorParameter::CreateFloat(name, defaultValue);
    }

    //! @brief Intパラメータを追加
    void AddParameter(const std::string& name, int defaultValue) {
        parameters_[name] = AnimatorParameter::CreateInt(name, defaultValue);
    }

    //! @brief Boolパラメータを追加
    void AddParameter(const std::string& name, bool defaultValue) {
        parameters_[name] = AnimatorParameter::CreateBool(name, defaultValue);
    }

    //! @brief Triggerパラメータを追加
    void AddTrigger(const std::string& name) {
        parameters_[name] = AnimatorParameter::CreateTrigger(name);
    }

    //! @brief パラメータを追加（汎用）
    void AddParameter(const AnimatorParameter& param) {
        parameters_[param.name] = param;
    }

    //! @brief パラメータが存在するか
    [[nodiscard]] bool HasParameter(const std::string& name) const {
        return parameters_.find(name) != parameters_.end();
    }

    //! @brief パラメータを取得
    [[nodiscard]] AnimatorParameter* GetParameter(const std::string& name) {
        auto it = parameters_.find(name);
        return (it != parameters_.end()) ? &it->second : nullptr;
    }

    [[nodiscard]] const AnimatorParameter* GetParameter(const std::string& name) const {
        auto it = parameters_.find(name);
        return (it != parameters_.end()) ? &it->second : nullptr;
    }

    //! @brief 全パラメータを取得
    [[nodiscard]] const std::unordered_map<std::string, AnimatorParameter>& GetParameters() const {
        return parameters_;
    }

    //! @brief パラメータのコピーを取得（実行時状態用）
    [[nodiscard]] std::unordered_map<std::string, AnimatorParameter> CloneParameters() const {
        return parameters_;
    }

    //========================================================================
    // パラメータ値の設定/取得
    //========================================================================

    void SetFloat(const std::string& name, float value) {
        auto* param = GetParameter(name);
        if (param) param->SetFloat(value);
    }

    void SetInt(const std::string& name, int value) {
        auto* param = GetParameter(name);
        if (param) param->SetInt(value);
    }

    void SetBool(const std::string& name, bool value) {
        auto* param = GetParameter(name);
        if (param) param->SetBool(value);
    }

    void SetTrigger(const std::string& name) {
        auto* param = GetParameter(name);
        if (param) param->SetTrigger();
    }

    void ResetTrigger(const std::string& name) {
        auto* param = GetParameter(name);
        if (param) param->ResetTrigger();
    }

    [[nodiscard]] float GetFloat(const std::string& name) const {
        auto* param = GetParameter(name);
        return param ? param->GetFloat() : 0.0f;
    }

    [[nodiscard]] int GetInt(const std::string& name) const {
        auto* param = GetParameter(name);
        return param ? param->GetInt() : 0;
    }

    [[nodiscard]] bool GetBool(const std::string& name) const {
        auto* param = GetParameter(name);
        return param ? param->GetBool() : false;
    }

    //========================================================================
    // Any State遷移
    //========================================================================

    //! @brief Any State遷移を追加（どのステートからでも遷移可能）
    //! @param layerIndex レイヤーインデックス
    //! @param destStateIndex 遷移先ステートインデックス
    //! @param duration ブレンド時間
    //! @return 追加された遷移への参照
    AnimatorTransition& AddAnyStateTransition(int layerIndex, int destStateIndex, float duration = 0.25f) {
        anyStateTransitions_.emplace_back(destStateIndex, duration);
        anyStateTransitions_.back().hasExitTime = false;  // Any StateはExit Timeなし
        anyStateLayerIndices_.push_back(layerIndex);
        return anyStateTransitions_.back();
    }

    //! @brief Any State遷移を取得
    [[nodiscard]] const std::vector<AnimatorTransition>& GetAnyStateTransitions() const {
        return anyStateTransitions_;
    }

    //! @brief Any State遷移のレイヤーインデックスを取得
    [[nodiscard]] int GetAnyStateTransitionLayer(size_t index) const {
        return (index < anyStateLayerIndices_.size()) ? anyStateLayerIndices_[index] : 0;
    }

    //========================================================================
    // ユーティリティ
    //========================================================================

    //! @brief コントローラーをクリア
    void Clear() {
        layers_.clear();
        parameters_.clear();
        anyStateTransitions_.clear();
        anyStateLayerIndices_.clear();
    }

    //! @brief 有効なコントローラーか
    [[nodiscard]] bool IsValid() const {
        return !layers_.empty() && !layers_[0].states.empty();
    }

private:
    std::vector<AnimatorLayer> layers_;                                 //!< レイヤー配列
    std::unordered_map<std::string, AnimatorParameter> parameters_;     //!< パラメータマップ

    // Any State遷移
    std::vector<AnimatorTransition> anyStateTransitions_;               //!< Any State遷移
    std::vector<int> anyStateLayerIndices_;                             //!< Any State遷移のレイヤー
};

using AnimatorControllerPtr = std::shared_ptr<AnimatorController>;
