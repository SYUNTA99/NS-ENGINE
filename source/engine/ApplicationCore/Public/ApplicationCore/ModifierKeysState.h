/// @file ModifierKeysState.h
/// @brief 修飾キービットマスクと状態スナップショット
#pragma once

#include <cstdint>

namespace NS
{
    // =========================================================================
    // ModifierKey
    // =========================================================================

    /// 修飾キービットマスク
    namespace ModifierKey
    {
        using Type = uint8_t;

        constexpr Type None = 0;
        constexpr Type Control = 1 << 0; // 0x01
        constexpr Type Alt = 1 << 1;     // 0x02
        constexpr Type Shift = 1 << 2;   // 0x04
        constexpr Type Command = 1 << 3; // 0x08

        /// bool値からビットマスクを合成
        inline Type FromBools(bool bControl, bool bAlt, bool bShift, bool bCommand)
        {
            Type Result = None;
            if (bControl)
                Result |= Control;
            if (bAlt)
                Result |= Alt;
            if (bShift)
                Result |= Shift;
            if (bCommand)
                Result |= Command;
            return Result;
        }
    } // namespace ModifierKey

    // =========================================================================
    // ModifierKeysState
    // =========================================================================

    /// 修飾キー状態のスナップショット
    class ModifierKeysState
    {
    public:
        /// 全false初期化
        ModifierKeysState() : m_bits(0) {}

        /// 9引数完全指定コンストラクタ
        ModifierKeysState(bool bInIsLeftShiftDown,
                          bool bInIsRightShiftDown,
                          bool bInIsLeftControlDown,
                          bool bInIsRightControlDown,
                          bool bInIsLeftAltDown,
                          bool bInIsRightAltDown,
                          bool bInIsLeftCommandDown,
                          bool bInIsRightCommandDown,
                          bool bInAreCapsLocked)
            : m_bits(0)
        {
            if (bInIsLeftShiftDown)
                m_bits |= Bit_LeftShift;
            if (bInIsRightShiftDown)
                m_bits |= Bit_RightShift;
            if (bInIsLeftControlDown)
                m_bits |= Bit_LeftControl;
            if (bInIsRightControlDown)
                m_bits |= Bit_RightControl;
            if (bInIsLeftAltDown)
                m_bits |= Bit_LeftAlt;
            if (bInIsRightAltDown)
                m_bits |= Bit_RightAlt;
            if (bInIsLeftCommandDown)
                m_bits |= Bit_LeftCommand;
            if (bInIsRightCommandDown)
                m_bits |= Bit_RightCommand;
            if (bInAreCapsLocked)
                m_bits |= Bit_CapsLock;
        }

        // 複合判定
        bool IsShiftDown() const { return (m_bits & (Bit_LeftShift | Bit_RightShift)) != 0; }
        bool IsControlDown() const { return (m_bits & (Bit_LeftControl | Bit_RightControl)) != 0; }
        bool IsAltDown() const { return (m_bits & (Bit_LeftAlt | Bit_RightAlt)) != 0; }
        bool IsCommandDown() const { return (m_bits & (Bit_LeftCommand | Bit_RightCommand)) != 0; }
        bool AreCapsLocked() const { return (m_bits & Bit_CapsLock) != 0; }

        /// ビットマスク判定（UE5タイポ保存: AreModifersDown）
        bool AreModifersDown(ModifierKey::Type ModifersMask) const
        {
            bool bResult = true;
            if ((ModifersMask & ModifierKey::Shift) != 0)
                bResult &= IsShiftDown();
            if ((ModifersMask & ModifierKey::Control) != 0)
                bResult &= IsControlDown();
            if ((ModifersMask & ModifierKey::Alt) != 0)
                bResult &= IsAltDown();
            if ((ModifersMask & ModifierKey::Command) != 0)
                bResult &= IsCommandDown();
            return bResult;
        }

        /// いずれかの修飾キーが押下
        bool AnyModifiersDown() const
        {
            // CapsLockを除く全修飾キービット
            constexpr uint16_t ModifierMask = Bit_LeftShift | Bit_RightShift | Bit_LeftControl | Bit_RightControl |
                                              Bit_LeftAlt | Bit_RightAlt | Bit_LeftCommand | Bit_RightCommand;
            return (m_bits & ModifierMask) != 0;
        }

        // 個別判定
        bool IsLeftShiftDown() const { return (m_bits & Bit_LeftShift) != 0; }
        bool IsRightShiftDown() const { return (m_bits & Bit_RightShift) != 0; }
        bool IsLeftControlDown() const { return (m_bits & Bit_LeftControl) != 0; }
        bool IsRightControlDown() const { return (m_bits & Bit_RightControl) != 0; }
        bool IsLeftAltDown() const { return (m_bits & Bit_LeftAlt) != 0; }
        bool IsRightAltDown() const { return (m_bits & Bit_RightAlt) != 0; }
        bool IsLeftCommandDown() const { return (m_bits & Bit_LeftCommand) != 0; }
        bool IsRightCommandDown() const { return (m_bits & Bit_RightCommand) != 0; }

    private:
        static constexpr uint16_t Bit_LeftShift = 1 << 0;
        static constexpr uint16_t Bit_RightShift = 1 << 1;
        static constexpr uint16_t Bit_LeftControl = 1 << 2;
        static constexpr uint16_t Bit_RightControl = 1 << 3;
        static constexpr uint16_t Bit_LeftAlt = 1 << 4;
        static constexpr uint16_t Bit_RightAlt = 1 << 5;
        static constexpr uint16_t Bit_LeftCommand = 1 << 6;
        static constexpr uint16_t Bit_RightCommand = 1 << 7;
        static constexpr uint16_t Bit_CapsLock = 1 << 8;

        uint16_t m_bits;
    };

} // namespace NS
