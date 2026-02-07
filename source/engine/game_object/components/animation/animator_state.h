//----------------------------------------------------------------------------
//! @file   animator_state.h
//! @brief  AnimatorState/Transition - ステートマシンの状態と遷移
//----------------------------------------------------------------------------
#pragma once


#include "animation_clip.h"
#include "animator_parameter.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

//============================================================================
//! @brief 遷移条件モード
//============================================================================
enum class ConditionMode {
    Greater,        //!< パラメータ > 閾値
    Less,           //!< パラメータ < 閾値
    Equals,         //!< パラメータ == 閾値
    NotEquals,      //!< パラメータ != 閾値
    If,             //!< Bool/Triggerがtrue
    IfNot           //!< Bool/Triggerがfalse
};

//============================================================================
//! @brief 遷移条件
//============================================================================
struct TransitionCondition {
    std::string parameterName;                      //!< パラメータ名
    ConditionMode mode = ConditionMode::If;         //!< 比較モード
    std::variant<float, int, bool> threshold;       //!< 閾値

    TransitionCondition() = default;

    TransitionCondition(const std::string& param, ConditionMode m)
        : parameterName(param)
        , mode(m)
        , threshold(true) {}

    TransitionCondition(const std::string& param, ConditionMode m, float thresh)
        : parameterName(param)
        , mode(m)
        , threshold(thresh) {}

    TransitionCondition(const std::string& param, ConditionMode m, int thresh)
        : parameterName(param)
        , mode(m)
        , threshold(thresh) {}

    //! @brief 条件を評価
    //! @param params パラメータマップ
    //! @return 条件が満たされていればtrue
    [[nodiscard]] bool Evaluate(
        const std::unordered_map<std::string, AnimatorParameter>& params) const {

        auto it = params.find(parameterName);
        if (it == params.end()) {
            return false;
        }

        const auto& param = it->second;

        switch (mode) {
            case ConditionMode::Greater:
                if (param.type == AnimatorParameterType::Float) {
                    return param.GetFloat() > std::get<float>(threshold);
                }
                if (param.type == AnimatorParameterType::Int) {
                    return param.GetInt() > std::get<int>(threshold);
                }
                break;

            case ConditionMode::Less:
                if (param.type == AnimatorParameterType::Float) {
                    return param.GetFloat() < std::get<float>(threshold);
                }
                if (param.type == AnimatorParameterType::Int) {
                    return param.GetInt() < std::get<int>(threshold);
                }
                break;

            case ConditionMode::Equals:
                if (param.type == AnimatorParameterType::Float) {
                    return std::abs(param.GetFloat() - std::get<float>(threshold)) < 0.0001f;
                }
                if (param.type == AnimatorParameterType::Int) {
                    return param.GetInt() == std::get<int>(threshold);
                }
                if (param.type == AnimatorParameterType::Bool) {
                    return param.GetBool() == std::get<bool>(threshold);
                }
                break;

            case ConditionMode::NotEquals:
                if (param.type == AnimatorParameterType::Float) {
                    return std::abs(param.GetFloat() - std::get<float>(threshold)) >= 0.0001f;
                }
                if (param.type == AnimatorParameterType::Int) {
                    return param.GetInt() != std::get<int>(threshold);
                }
                if (param.type == AnimatorParameterType::Bool) {
                    return param.GetBool() != std::get<bool>(threshold);
                }
                break;

            case ConditionMode::If:
                return param.GetBool();

            case ConditionMode::IfNot:
                return !param.GetBool();
        }

        return false;
    }

    //------------------------------------------------------------------------
    // ファクトリ
    //------------------------------------------------------------------------

    //! @brief Float比較条件を作成
    static TransitionCondition FloatGreater(const std::string& param, float threshold) {
        return TransitionCondition(param, ConditionMode::Greater, threshold);
    }

    static TransitionCondition FloatLess(const std::string& param, float threshold) {
        return TransitionCondition(param, ConditionMode::Less, threshold);
    }

    //! @brief Int比較条件を作成
    static TransitionCondition IntEquals(const std::string& param, int threshold) {
        return TransitionCondition(param, ConditionMode::Equals, threshold);
    }

    //! @brief Bool条件を作成
    static TransitionCondition BoolTrue(const std::string& param) {
        return TransitionCondition(param, ConditionMode::If);
    }

    static TransitionCondition BoolFalse(const std::string& param) {
        return TransitionCondition(param, ConditionMode::IfNot);
    }

    //! @brief Trigger条件を作成
    static TransitionCondition Trigger(const std::string& param) {
        return TransitionCondition(param, ConditionMode::If);
    }
};

//============================================================================
//! @brief AnimatorTransition - 状態遷移
//============================================================================
struct AnimatorTransition {
    int destinationStateIndex = -1;                 //!< 遷移先ステートインデックス
    float duration = 0.25f;                         //!< ブレンド時間（秒）
    float exitTime = 0.0f;                          //!< 終了時間（正規化、0〜1）
    bool hasExitTime = true;                        //!< 終了時間を使用するか
    float offset = 0.0f;                            //!< 遷移先の開始オフセット（正規化）
    bool canTransitionToSelf = false;               //!< 自身への遷移を許可

    std::vector<TransitionCondition> conditions;    //!< 遷移条件

    AnimatorTransition() = default;

    AnimatorTransition(int destIndex, float blendDuration = 0.25f)
        : destinationStateIndex(destIndex)
        , duration(blendDuration) {}

    //! @brief 条件を追加
    AnimatorTransition& AddCondition(const TransitionCondition& condition) {
        conditions.push_back(condition);
        return *this;
    }

    //! @brief 全条件を評価（AND条件）
    //! @param params パラメータマップ
    //! @param normalizedTime 現在の正規化時間
    //! @return 遷移可能ならtrue
    [[nodiscard]] bool EvaluateConditions(
        const std::unordered_map<std::string, AnimatorParameter>& params,
        float normalizedTime = 0.0f) const {

        // Exit Timeチェック
        if (hasExitTime && normalizedTime < exitTime) {
            return false;
        }

        // 条件が空の場合、Exit Timeのみで判定
        if (conditions.empty()) {
            return hasExitTime;
        }

        // 全条件をANDで評価
        for (const auto& condition : conditions) {
            if (!condition.Evaluate(params)) {
                return false;
            }
        }
        return true;
    }

    //! @brief 使用されたトリガーをリセット
    void ConsumeTriggers(std::unordered_map<std::string, AnimatorParameter>& params) const {
        for (const auto& condition : conditions) {
            auto it = params.find(condition.parameterName);
            if (it != params.end() && it->second.type == AnimatorParameterType::Trigger) {
                it->second.ResetTrigger();
            }
        }
    }
};

//============================================================================
//! @brief AnimatorState - アニメーション状態
//============================================================================
struct AnimatorState {
    std::string name;                               //!< ステート名
    AnimationClipPtr clip;                          //!< アニメーションクリップ
    float speed = 1.0f;                             //!< 再生速度
    std::string tag;                                //!< タグ（グループ分け用）
    bool loop = true;                               //!< ループ再生

    std::vector<AnimatorTransition> transitions;    //!< 遷移リスト

    // 将来拡張用
    // std::shared_ptr<BlendTree> blendTree;        //!< ブレンドツリー（Motion代替）
    // std::string speedParameterName;              //!< 速度を制御するパラメータ

    AnimatorState() = default;

    AnimatorState(const std::string& stateName, AnimationClipPtr animClip = nullptr)
        : name(stateName)
        , clip(std::move(animClip)) {}

    //! @brief 遷移を追加
    AnimatorTransition& AddTransition(int destIndex, float duration = 0.25f) {
        transitions.emplace_back(destIndex, duration);
        return transitions.back();
    }

    //! @brief ステートの長さを取得（秒）
    [[nodiscard]] float GetLength() const {
        return clip ? clip->duration : 0.0f;
    }

    //! @brief 有効なステートか
    [[nodiscard]] bool IsValid() const {
        return clip && clip->IsValid();
    }

    //! @brief 名前ハッシュを取得（高速比較用）
    [[nodiscard]] size_t GetNameHash() const {
        return std::hash<std::string>{}(name);
    }
};
