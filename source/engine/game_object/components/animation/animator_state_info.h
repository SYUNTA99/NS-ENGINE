//----------------------------------------------------------------------------
//! @file   animator_state_info.h
//! @brief  AnimatorStateInfo - アニメーター状態情報
//----------------------------------------------------------------------------
#pragma once


#include <string>
#include <functional>

//============================================================================
//! @brief AnimatorStateInfo - アニメーター状態情報
//!
//! Animator::GetCurrentAnimatorStateInfo()で取得される、
//! 現在再生中のステートに関する情報。
//!
//! @code
//! AnimatorStateInfo info = animator->GetCurrentAnimatorStateInfo(0);
//!
//! if (info.IsName("Attack")) {
//!     // 攻撃中の処理
//! }
//!
//! if (info.normalizedTime > 0.9f && !info.loop) {
//!     // アニメーション終了間近
//! }
//!
//! if (info.IsTag("Locomotion")) {
//!     // 移動系アニメーション
//! }
//! @endcode
//============================================================================
struct AnimatorStateInfo {
    std::string stateName;          //!< ステート名
    size_t stateNameHash = 0;       //!< ステート名ハッシュ（高速比較用）
    std::string tag;                //!< ステートタグ

    float normalizedTime = 0.0f;    //!< 正規化時間 [0-1]（ループ時は1を超える）
    float length = 0.0f;            //!< クリップ長（秒）
    float speed = 1.0f;             //!< 再生速度
    bool loop = false;              //!< ループ再生か

    int layerIndex = 0;             //!< レイヤーインデックス
    int stateIndex = -1;            //!< ステートインデックス

    AnimatorStateInfo() = default;

    //! @brief ステート名で比較
    //! @param name 比較するステート名
    //! @return 一致すればtrue
    [[nodiscard]] bool IsName(const std::string& name) const {
        return stateName == name;
    }

    //! @brief ステート名ハッシュで比較（高速）
    //! @param hash 比較するハッシュ値
    //! @return 一致すればtrue
    [[nodiscard]] bool IsName(size_t hash) const {
        return stateNameHash == hash;
    }

    //! @brief タグで比較
    //! @param tagName 比較するタグ名
    //! @return 一致すればtrue
    [[nodiscard]] bool IsTag(const std::string& tagName) const {
        return tag == tagName;
    }

    //! @brief 現在の再生時間（秒）を取得
    [[nodiscard]] float GetCurrentTime() const {
        return normalizedTime * length;
    }

    //! @brief 残り時間（秒）を取得（非ループ時のみ有効）
    [[nodiscard]] float GetRemainingTime() const {
        if (loop || length <= 0.0f) return 0.0f;
        float remaining = length - (normalizedTime * length);
        return (remaining > 0.0f) ? remaining : 0.0f;
    }

    //! @brief ループ回数を取得
    [[nodiscard]] int GetLoopCount() const {
        return loop ? static_cast<int>(normalizedTime) : 0;
    }

    //! @brief ステート名ハッシュを計算
    static size_t HashName(const std::string& name) {
        return std::hash<std::string>{}(name);
    }
};

//============================================================================
//! @brief AnimatorTransitionInfo - 遷移情報
//!
//! 遷移中の状態に関する情報。
//============================================================================
struct AnimatorTransitionInfo {
    std::string sourceStateName;        //!< 遷移元ステート名
    std::string destinationStateName;   //!< 遷移先ステート名
    float normalizedTime = 0.0f;        //!< 遷移の進行度 [0-1]
    float duration = 0.0f;              //!< 遷移時間（秒）

    int sourceStateIndex = -1;          //!< 遷移元ステートインデックス
    int destinationStateIndex = -1;     //!< 遷移先ステートインデックス

    AnimatorTransitionInfo() = default;

    //! @brief 遷移元ステート名で比較
    [[nodiscard]] bool IsSourceState(const std::string& name) const {
        return sourceStateName == name;
    }

    //! @brief 遷移先ステート名で比較
    [[nodiscard]] bool IsDestinationState(const std::string& name) const {
        return destinationStateName == name;
    }
};

//============================================================================
//! @brief AnimatorClipInfo - クリップ情報
//!
//! 再生中のアニメーションクリップに関する情報。
//============================================================================
struct AnimatorClipInfo {
    std::string clipName;               //!< クリップ名
    float weight = 1.0f;                //!< ブレンドウェイト

    AnimatorClipInfo() = default;

    AnimatorClipInfo(const std::string& name, float w = 1.0f)
        : clipName(name)
        , weight(w) {}
};
