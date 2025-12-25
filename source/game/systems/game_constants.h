//----------------------------------------------------------------------------
//! @file   game_constants.h
//! @brief  ゲーム全体で使用する共通定数
//----------------------------------------------------------------------------
#pragma once

namespace GameConstants
{
    //------------------------------------------------------------------------
    // Love縁関連
    //------------------------------------------------------------------------

    //! @brief Love縁追従開始距離（この距離を超えると追従開始）
    constexpr float kLoveFollowStartDistance = 100.0f;

    //! @brief Love縁追従速度（プレイヤーと同速）
    constexpr float kLoveFollowSpeed = 200.0f;

    //! @brief Love縁攻撃中断距離（この距離を超えると攻撃中断・追従優先）
    constexpr float kLoveInterruptDistance = 350.0f;

    //! @brief Love縁停止距離（この距離以内で停止）
    constexpr float kLoveStopDistance = 5.0f;

    //------------------------------------------------------------------------
    // 攻撃関連
    //------------------------------------------------------------------------

    //! @brief 最小近接攻撃範囲
    constexpr float kMinMeleeAttackRange = 50.0f;

}  // namespace GameConstants
