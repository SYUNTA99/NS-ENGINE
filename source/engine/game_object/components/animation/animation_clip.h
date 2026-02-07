//----------------------------------------------------------------------------
//! @file   animation_clip.h
//! @brief  AnimationClip - アニメーションクリップ（キーフレームデータ）
//----------------------------------------------------------------------------
#pragma once


#include "engine/math/math_types.h"
#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <algorithm>
#include <cmath>

//============================================================================
//! @brief ラップモード
//============================================================================
enum class WrapMode {
    Once,           //!< 1回再生して停止
    Loop,           //!< ループ再生
    PingPong,       //!< 往復再生
    ClampForever    //!< 最終フレームで停止
};

//============================================================================
//! @brief キーフレーム（テンプレート）
//============================================================================
template<typename T>
struct Keyframe {
    float time;     //!< 時間（秒）
    T value;        //!< 値

    Keyframe() = default;
    Keyframe(float t, const T& v) : time(t), value(v) {}
};

// 型エイリアス
using PositionKey = Keyframe<Vector3>;
using RotationKey = Keyframe<Quaternion>;
using ScaleKey = Keyframe<Vector3>;

//============================================================================
//! @brief アニメーションイベント
//============================================================================
struct AnimationEvent {
    float time;                                         //!< 発火時間（秒）
    std::string functionName;                           //!< 関数名
    std::variant<int, float, std::string> parameter;    //!< パラメータ

    AnimationEvent() = default;
    AnimationEvent(float t, const std::string& func)
        : time(t), functionName(func), parameter(0) {}

    template<typename T>
    AnimationEvent(float t, const std::string& func, const T& param)
        : time(t), functionName(func), parameter(param) {}
};

//============================================================================
//! @brief 補間ユーティリティ
//============================================================================
namespace AnimationInterp {

//! @brief 線形補間
template<typename T>
T Lerp(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}

//! @brief Vector3の線形補間
inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
    return Vector3::Lerp(a, b, t);
}

//! @brief Quaternionの球面線形補間
inline Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) {
    return Quaternion::Slerp(a, b, t);
}

//! @brief キーフレーム配列から補間サンプリング
template<typename T, typename InterpFunc>
T SampleKeyframes(const std::vector<Keyframe<T>>& keys, float time, InterpFunc interp) {
    if (keys.empty()) {
        return T{};
    }
    if (keys.size() == 1) {
        return keys[0].value;
    }

    // 時間が最初より前
    if (time <= keys.front().time) {
        return keys.front().value;
    }
    // 時間が最後より後
    if (time >= keys.back().time) {
        return keys.back().value;
    }

    // 二分探索でキーフレームを見つける
    size_t left = 0;
    size_t right = keys.size() - 1;
    while (left < right - 1) {
        size_t mid = (left + right) / 2;
        if (keys[mid].time <= time) {
            left = mid;
        } else {
            right = mid;
        }
    }

    // 補間係数を計算
    const auto& key0 = keys[left];
    const auto& key1 = keys[right];
    float dt = key1.time - key0.time;
    float t = (dt > 0.0001f) ? (time - key0.time) / dt : 0.0f;

    return interp(key0.value, key1.value, t);
}

} // namespace AnimationInterp

//============================================================================
//! @brief ボーンチャンネル（1ボーン分のアニメーションデータ）
//============================================================================
struct BoneChannel {
    int boneIndex = -1;                             //!< 対象ボーンインデックス
    std::string boneName;                           //!< ボーン名（ロード時のマッピング用）

    std::vector<PositionKey> positionKeys;          //!< 位置キーフレーム
    std::vector<RotationKey> rotationKeys;          //!< 回転キーフレーム
    std::vector<ScaleKey> scaleKeys;                //!< スケールキーフレーム

    //! @brief 指定時間でのTransform行列をサンプリング
    //! @param time サンプリング時間（秒）
    //! @return Transform行列
    [[nodiscard]] Matrix SampleAt(float time) const {
        // 位置
        Vector3 position = Vector3::Zero;
        if (!positionKeys.empty()) {
            position = AnimationInterp::SampleKeyframes(
                positionKeys, time,
                [](const Vector3& a, const Vector3& b, float t) {
                    return AnimationInterp::Lerp(a, b, t);
                });
        }

        // 回転
        Quaternion rotation = Quaternion::Identity;
        if (!rotationKeys.empty()) {
            rotation = AnimationInterp::SampleKeyframes(
                rotationKeys, time,
                [](const Quaternion& a, const Quaternion& b, float t) {
                    return AnimationInterp::Slerp(a, b, t);
                });
        }

        // スケール
        Vector3 scale = Vector3::One;
        if (!scaleKeys.empty()) {
            scale = AnimationInterp::SampleKeyframes(
                scaleKeys, time,
                [](const Vector3& a, const Vector3& b, float t) {
                    return AnimationInterp::Lerp(a, b, t);
                });
        }

        // TRS行列を構築
        return Matrix::CreateScale(scale) *
               Matrix::CreateFromQuaternion(rotation) *
               Matrix::CreateTranslation(position);
    }

    //! @brief キーフレームが存在するか
    [[nodiscard]] bool HasKeys() const noexcept {
        return !positionKeys.empty() || !rotationKeys.empty() || !scaleKeys.empty();
    }
};

//============================================================================
//! @brief AnimationClip - アニメーションクリップ
//!
//! スケルタルアニメーションのキーフレームデータを保持。
//! Animatorコンポーネントがクリップをサンプリングしてボーンを更新する。
//!
//! @code
//! auto clip = std::make_shared<AnimationClip>();
//! clip->name = "Walk";
//! clip->duration = 1.0f;
//! clip->wrapMode = WrapMode::Loop;
//!
//! // ボーンチャンネルを追加
//! BoneChannel& channel = clip->AddChannel(0, "Root");
//! channel.positionKeys.push_back({0.0f, Vector3(0, 0, 0)});
//! channel.positionKeys.push_back({0.5f, Vector3(0, 0.5f, 0)});
//! channel.positionKeys.push_back({1.0f, Vector3(0, 0, 0)});
//!
//! // サンプリング
//! std::vector<Matrix> pose;
//! clip->SamplePose(0.25f, pose);
//! @endcode
//============================================================================
class AnimationClip {
public:
    std::string name;                               //!< クリップ名
    float duration = 0.0f;                          //!< 再生時間（秒）
    float frameRate = 30.0f;                        //!< フレームレート
    WrapMode wrapMode = WrapMode::Loop;             //!< ラップモード

    std::vector<BoneChannel> channels;              //!< ボーンチャンネル配列
    std::vector<AnimationEvent> events;             //!< アニメーションイベント

    //========================================================================
    // チャンネル管理
    //========================================================================

    //! @brief ボーンチャンネルを追加
    //! @param boneIndex ボーンインデックス
    //! @param boneName ボーン名
    //! @return 追加されたチャンネルへの参照
    BoneChannel& AddChannel(int boneIndex, const std::string& boneName = "") {
        channels.emplace_back();
        auto& channel = channels.back();
        channel.boneIndex = boneIndex;
        channel.boneName = boneName;
        return channel;
    }

    //! @brief ボーンインデックスからチャンネルを検索
    //! @param boneIndex ボーンインデックス
    //! @return チャンネルポインタ（見つからない場合nullptr）
    [[nodiscard]] BoneChannel* FindChannel(int boneIndex) {
        for (auto& channel : channels) {
            if (channel.boneIndex == boneIndex) {
                return &channel;
            }
        }
        return nullptr;
    }

    //! @brief ボーン名からチャンネルを検索
    [[nodiscard]] BoneChannel* FindChannelByName(const std::string& boneName) {
        for (auto& channel : channels) {
            if (channel.boneName == boneName) {
                return &channel;
            }
        }
        return nullptr;
    }

    //========================================================================
    // サンプリング
    //========================================================================

    //! @brief 時間をラップモードに従って正規化
    //! @param time 入力時間
    //! @return 正規化された時間
    [[nodiscard]] float WrapTime(float time) const {
        if (duration <= 0.0f) return 0.0f;

        switch (wrapMode) {
            case WrapMode::Once:
                return std::clamp(time, 0.0f, duration);

            case WrapMode::Loop: {
                float t = std::fmod(time, duration);
                return (t < 0.0f) ? t + duration : t;
            }

            case WrapMode::PingPong: {
                float t = std::fmod(time, duration * 2.0f);
                if (t < 0.0f) t += duration * 2.0f;
                return (t > duration) ? (duration * 2.0f - t) : t;
            }

            case WrapMode::ClampForever:
                return std::clamp(time, 0.0f, duration);
        }
        return time;
    }

    //! @brief 全ボーンのポーズをサンプリング
    //! @param time サンプリング時間（秒）
    //! @param outLocalTransforms 出力行列配列（ボーン数分のサイズが必要）
    //!
    //! @note outLocalTransformsは事前にボーン数分のサイズでIdentity行列で
    //!       初期化しておくこと。アニメーションデータがないボーンは変更されない。
    void SamplePose(float time, std::vector<Matrix>& outLocalTransforms) const {
        float wrappedTime = WrapTime(time);

        for (const auto& channel : channels) {
            if (channel.boneIndex >= 0 &&
                channel.boneIndex < static_cast<int>(outLocalTransforms.size())) {
                if (channel.HasKeys()) {
                    outLocalTransforms[channel.boneIndex] = channel.SampleAt(wrappedTime);
                }
            }
        }
    }

    //! @brief 特定のボーンのTransformをサンプリング
    //! @param boneIndex ボーンインデックス
    //! @param time サンプリング時間
    //! @return Transform行列（チャンネルが見つからない場合Identity）
    [[nodiscard]] Matrix SampleBone(int boneIndex, float time) const {
        float wrappedTime = WrapTime(time);

        for (const auto& channel : channels) {
            if (channel.boneIndex == boneIndex) {
                return channel.SampleAt(wrappedTime);
            }
        }
        return Matrix::Identity;
    }

    //========================================================================
    // イベント
    //========================================================================

    //! @brief イベントを追加
    //! @param time 発火時間（秒）
    //! @param functionName 関数名
    void AddEvent(float time, const std::string& functionName) {
        events.emplace_back(time, functionName);
        SortEvents();
    }

    //! @brief イベントを追加（パラメータ付き）
    template<typename T>
    void AddEvent(float time, const std::string& functionName, const T& parameter) {
        events.emplace_back(time, functionName, parameter);
        SortEvents();
    }

    //! @brief 時間範囲内のイベントを取得
    //! @param prevTime 前回の時間
    //! @param currTime 現在の時間
    //! @param outEvents 出力イベント配列
    void GetEventsInRange(float prevTime, float currTime,
                          std::vector<const AnimationEvent*>& outEvents) const {
        outEvents.clear();

        // ラップを考慮
        if (wrapMode == WrapMode::Loop && currTime < prevTime) {
            // ループした場合: prevTime〜duration + 0〜currTime
            for (const auto& event : events) {
                if ((event.time >= prevTime && event.time <= duration) ||
                    (event.time >= 0.0f && event.time <= currTime)) {
                    outEvents.push_back(&event);
                }
            }
        } else {
            // 通常: prevTime〜currTime
            for (const auto& event : events) {
                if (event.time > prevTime && event.time <= currTime) {
                    outEvents.push_back(&event);
                }
            }
        }
    }

    //========================================================================
    // ユーティリティ
    //========================================================================

    //! @brief 正規化時間を実時間に変換
    [[nodiscard]] float NormalizedToTime(float normalized) const {
        return normalized * duration;
    }

    //! @brief 実時間を正規化時間に変換
    [[nodiscard]] float TimeToNormalized(float time) const {
        return (duration > 0.0f) ? (time / duration) : 0.0f;
    }

    //! @brief クリップが有効か
    [[nodiscard]] bool IsValid() const noexcept {
        return duration > 0.0f && !channels.empty();
    }

private:
    void SortEvents() {
        std::sort(events.begin(), events.end(),
            [](const AnimationEvent& a, const AnimationEvent& b) {
                return a.time < b.time;
            });
    }
};

using AnimationClipPtr = std::shared_ptr<AnimationClip>;
