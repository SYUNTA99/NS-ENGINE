//----------------------------------------------------------------------------
//! @file   animator_parameter.h
//! @brief  AnimatorParameter - アニメーターパラメータ
//----------------------------------------------------------------------------
#pragma once


#include <string>
#include <variant>

//============================================================================
//! @brief パラメータ型
//============================================================================
enum class AnimatorParameterType {
    Float,      //!< 浮動小数点
    Int,        //!< 整数
    Bool,       //!< 真偽値
    Trigger     //!< トリガー（自動リセット）
};

//============================================================================
//! @brief AnimatorParameter - アニメーターパラメータ
//!
//! Animatorのステートマシンで使用するパラメータ。
//! 遷移条件の評価やBlendTreeのウェイト計算に使用される。
//!
//! @code
//! AnimatorParameter speed;
//! speed.name = "Speed";
//! speed.type = AnimatorParameterType::Float;
//! speed.value = 0.0f;
//!
//! AnimatorParameter isGrounded;
//! isGrounded.name = "IsGrounded";
//! isGrounded.type = AnimatorParameterType::Bool;
//! isGrounded.value = true;
//!
//! AnimatorParameter jumpTrigger;
//! jumpTrigger.name = "Jump";
//! jumpTrigger.type = AnimatorParameterType::Trigger;
//! jumpTrigger.value = false;
//! @endcode
//============================================================================
struct AnimatorParameter {
    std::string name;                           //!< パラメータ名
    AnimatorParameterType type;                 //!< パラメータ型
    std::variant<float, int, bool> value;       //!< 値

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    AnimatorParameter()
        : type(AnimatorParameterType::Float)
        , value(0.0f) {}

    AnimatorParameter(const std::string& paramName, float defaultValue)
        : name(paramName)
        , type(AnimatorParameterType::Float)
        , value(defaultValue) {}

    AnimatorParameter(const std::string& paramName, int defaultValue)
        : name(paramName)
        , type(AnimatorParameterType::Int)
        , value(defaultValue) {}

    AnimatorParameter(const std::string& paramName, bool defaultValue, bool isTrigger = false)
        : name(paramName)
        , type(isTrigger ? AnimatorParameterType::Trigger : AnimatorParameterType::Bool)
        , value(defaultValue) {}

    //------------------------------------------------------------------------
    // 値の取得
    //------------------------------------------------------------------------

    //! @brief Float値を取得
    [[nodiscard]] float GetFloat() const {
        if (auto* val = std::get_if<float>(&value)) {
            return *val;
        }
        if (auto* val = std::get_if<int>(&value)) {
            return static_cast<float>(*val);
        }
        return 0.0f;
    }

    //! @brief Int値を取得
    [[nodiscard]] int GetInt() const {
        if (auto* val = std::get_if<int>(&value)) {
            return *val;
        }
        if (auto* val = std::get_if<float>(&value)) {
            return static_cast<int>(*val);
        }
        return 0;
    }

    //! @brief Bool値を取得
    [[nodiscard]] bool GetBool() const {
        if (auto* val = std::get_if<bool>(&value)) {
            return *val;
        }
        if (auto* val = std::get_if<int>(&value)) {
            return *val != 0;
        }
        if (auto* val = std::get_if<float>(&value)) {
            return *val != 0.0f;
        }
        return false;
    }

    //------------------------------------------------------------------------
    // 値の設定
    //------------------------------------------------------------------------

    //! @brief Float値を設定
    void SetFloat(float val) {
        if (type == AnimatorParameterType::Float) {
            value = val;
        }
    }

    //! @brief Int値を設定
    void SetInt(int val) {
        if (type == AnimatorParameterType::Int) {
            value = val;
        }
    }

    //! @brief Bool値を設定
    void SetBool(bool val) {
        if (type == AnimatorParameterType::Bool || type == AnimatorParameterType::Trigger) {
            value = val;
        }
    }

    //! @brief トリガーを発火
    void SetTrigger() {
        if (type == AnimatorParameterType::Trigger) {
            value = true;
        }
    }

    //! @brief トリガーをリセット
    void ResetTrigger() {
        if (type == AnimatorParameterType::Trigger) {
            value = false;
        }
    }

    //------------------------------------------------------------------------
    // ファクトリ
    //------------------------------------------------------------------------

    //! @brief Floatパラメータを作成
    static AnimatorParameter CreateFloat(const std::string& name, float defaultValue = 0.0f) {
        return AnimatorParameter(name, defaultValue);
    }

    //! @brief Intパラメータを作成
    static AnimatorParameter CreateInt(const std::string& name, int defaultValue = 0) {
        return AnimatorParameter(name, defaultValue);
    }

    //! @brief Boolパラメータを作成
    static AnimatorParameter CreateBool(const std::string& name, bool defaultValue = false) {
        return AnimatorParameter(name, defaultValue, false);
    }

    //! @brief Triggerパラメータを作成
    static AnimatorParameter CreateTrigger(const std::string& name) {
        return AnimatorParameter(name, false, true);
    }
};
