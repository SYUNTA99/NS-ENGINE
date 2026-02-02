//----------------------------------------------------------------------------
//! @file   animator_data.h
//! @brief  ECS AnimatorData - スプライトアニメーションデータ構造体
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"
#include <array>
#include <cstdint>

namespace ECS {

//============================================================================
//! @brief アニメーションデータ（POD構造体）
//!
//! スプライトシートアニメーションを管理するデータ構造。
//! AnimatorSystem によってフレームが更新され、
//! SpriteRenderSystem が UV 座標に反映する。
//!
//! @note メモリレイアウト: 48 bytes, 16B境界アライン
//============================================================================
struct alignas(16) AnimatorData : public IComponentData {
    //------------------------------------------------------------------------
    // 行ごとの設定（最大16行）
    //------------------------------------------------------------------------
    std::array<uint8_t, 16> rowFrameCounts{};    //!< 行ごとの有効フレーム数
    std::array<uint8_t, 16> rowFrameIntervals{}; //!< 行ごとのフレーム間隔

    //------------------------------------------------------------------------
    // UVキャッシュ
    //------------------------------------------------------------------------
    Vector2 uvSize = Vector2::One;               //!< 1フレームのUVサイズ

    //------------------------------------------------------------------------
    // 状態パック
    //------------------------------------------------------------------------
    uint8_t rowCount = 1;       //!< スプライトシート縦分割数
    uint8_t colCount = 1;       //!< スプライトシート横分割数
    uint8_t currentRow = 0;     //!< 現在のアニメーション行
    uint8_t currentCol = 0;     //!< 現在のフレーム列
    uint8_t frameInterval = 1;  //!< デフォルトフレーム間隔
    uint8_t counter = 0;        //!< 経過フレームカウンタ
    uint8_t flags = 0x06;       //!< Mirror(0x01) | Playing(0x02) | Looping(0x04)
    uint8_t _pad0 = 0;

    //------------------------------------------------------------------------
    // フラグ定数
    //------------------------------------------------------------------------
    static constexpr uint8_t kFlagMirror  = 0x01;
    static constexpr uint8_t kFlagPlaying = 0x02;
    static constexpr uint8_t kFlagLooping = 0x04;

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief 再生中かどうか
    [[nodiscard]] bool IsPlaying() const noexcept {
        return (flags & kFlagPlaying) != 0;
    }

    //! @brief ループ再生かどうか
    [[nodiscard]] bool IsLooping() const noexcept {
        return (flags & kFlagLooping) != 0;
    }

    //! @brief 左右反転かどうか
    [[nodiscard]] bool GetMirror() const noexcept {
        return (flags & kFlagMirror) != 0;
    }

    //! @brief 再生状態を設定
    void SetPlaying(bool playing) noexcept {
        flags = playing ? (flags | kFlagPlaying) : (flags & ~kFlagPlaying);
    }

    //! @brief ループ状態を設定
    void SetLooping(bool looping) noexcept {
        flags = looping ? (flags | kFlagLooping) : (flags & ~kFlagLooping);
    }

    //! @brief 左右反転を設定
    void SetMirror(bool mirror) noexcept {
        flags = mirror ? (flags | kFlagMirror) : (flags & ~kFlagMirror);
    }

    //! @brief 再生開始
    void Play() noexcept {
        SetPlaying(true);
        counter = 0;
    }

    //! @brief 停止
    void Stop() noexcept {
        SetPlaying(false);
    }

    //! @brief アニメーション行を変更
    void SetRow(uint8_t row) noexcept {
        if (row < rowCount) {
            currentRow = row;
            currentCol = 0;
            counter = 0;
        }
    }

    //! @brief 現在行の有効フレーム数を取得
    [[nodiscard]] uint8_t GetCurrentRowFrameCount() const noexcept {
        uint8_t count = rowFrameCounts[currentRow];
        return (count > 0) ? count : colCount;
    }

    //! @brief 現在行のフレーム間隔を取得
    [[nodiscard]] uint8_t GetCurrentRowInterval() const noexcept {
        uint8_t interval = rowFrameIntervals[currentRow];
        return (interval > 0) ? interval : frameInterval;
    }

    //! @brief 現在フレームのUVオフセットを計算
    [[nodiscard]] Vector2 GetUVOffset() const noexcept {
        float u = uvSize.x * static_cast<float>(currentCol);
        float v = uvSize.y * static_cast<float>(currentRow);
        return GetMirror() ? Vector2(u + uvSize.x, v) : Vector2(u, v);
    }

    //! @brief 現在フレームのUVサイズを計算（反転考慮）
    [[nodiscard]] Vector2 GetUVSize() const noexcept {
        return GetMirror() ? Vector2(-uvSize.x, uvSize.y) : uvSize;
    }

    //------------------------------------------------------------------------
    // 初期化
    //------------------------------------------------------------------------

    //! @brief シート分割を設定してUVサイズを計算
    void Setup(uint8_t rows, uint8_t cols) noexcept {
        rowCount = rows;
        colCount = cols;
        uvSize.x = 1.0f / static_cast<float>(cols);
        uvSize.y = 1.0f / static_cast<float>(rows);
    }

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    AnimatorData() = default;

    AnimatorData(uint8_t rows, uint8_t cols, uint8_t interval = 1)
        : rowCount(rows)
        , colCount(cols)
        , frameInterval(interval)
        , uvSize(1.0f / static_cast<float>(cols), 1.0f / static_cast<float>(rows)) {}
};

ECS_COMPONENT(AnimatorData);

} // namespace ECS
