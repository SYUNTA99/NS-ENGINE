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

        constexpr Type kNone = 0;
        constexpr Type kControl = 1 << 0; // 0x01
        constexpr Type kAlt = 1 << 1;     // 0x02
        constexpr Type kShift = 1 << 2;   // 0x04
        constexpr Type kCommand = 1 << 3; // 0x08

        /// bool値からビットマスクを合成
        inline Type FromBools(bool bControl, bool bAlt, bool bShift, bool bCommand)
        {
            Type result = kNone;
            if (bControl) {
                result |= kControl;
}
            if (bAlt) {
                result |= kAlt;
}
            if (bShift) {
                result |= kShift;
}
            if (bCommand) {
                result |= kCommand;
}
            return result;
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
            if (bInIsLeftShiftDown) {
                m_bits |= kBitLeftShift;
}
            if (bInIsRightShiftDown) {
                m_bits |= kBitRightShift;
}
            if (bInIsLeftControlDown) {
                m_bits |= kBitLeftControl;
}
            if (bInIsRightControlDown) {
                m_bits |= kBitRightControl;
}
            if (bInIsLeftAltDown) {
                m_bits |= kBitLeftAlt;
}
            if (bInIsRightAltDown) {
                m_bits |= kBitRightAlt;
}
            if (bInIsLeftCommandDown) {
                m_bits |= kBitLeftCommand;
}
            if (bInIsRightCommandDown) {
                m_bits |= kBitRightCommand;
}
            if (bInAreCapsLocked) {
                m_bits |= kBitCapsLock;
}
        }

        // 複合判定
        [[nodiscard]] bool IsShiftDown() const { return (m_bits & (kBitLeftShift | kBitRightShift)) != 0; }
        [[nodiscard]] bool IsControlDown() const { return (m_bits & (kBitLeftControl | kBitRightControl)) != 0; }
        [[nodiscard]] bool IsAltDown() const { return (m_bits & (kBitLeftAlt | kBitRightAlt)) != 0; }
        [[nodiscard]] bool IsCommandDown() const { return (m_bits & (kBitLeftCommand | kBitRightCommand)) != 0; }
        [[nodiscard]] bool AreCapsLocked() const { return (m_bits & kBitCapsLock) != 0; }

        /// ビットマスク判定（UE5タイポ保存: AreModifersDown）
        [[nodiscard]] bool AreModifersDown(ModifierKey::Type modifersMask) const
        {
            bool bResult = true;
            if ((modifersMask & ModifierKey::kShift) != 0) {
                bResult &= IsShiftDown();
}
            if ((modifersMask & ModifierKey::kControl) != 0) {
                bResult &= IsControlDown();
}
            if ((modifersMask & ModifierKey::kAlt) != 0) {
                bResult &= IsAltDown();
}
            if ((modifersMask & ModifierKey::kCommand) != 0) {
                bResult &= IsCommandDown();
}
            return bResult;
        }

        /// いずれかの修飾キーが押下
        [[nodiscard]] bool AnyModifiersDown() const
        {
            // CapsLockを除く全修飾キービット
            constexpr uint16_t kModifierMask = kBitLeftShift | kBitRightShift | kBitLeftControl | kBitRightControl |
                                              kBitLeftAlt | kBitRightAlt | kBitLeftCommand | kBitRightCommand;
            return (m_bits & kModifierMask) != 0;
        }

        // 個別判定
        [[nodiscard]] bool IsLeftShiftDown() const { return (m_bits & kBitLeftShift) != 0; }
        [[nodiscard]] bool IsRightShiftDown() const { return (m_bits & kBitRightShift) != 0; }
        [[nodiscard]] bool IsLeftControlDown() const { return (m_bits & kBitLeftControl) != 0; }
        [[nodiscard]] bool IsRightControlDown() const { return (m_bits & kBitRightControl) != 0; }
        [[nodiscard]] bool IsLeftAltDown() const { return (m_bits & kBitLeftAlt) != 0; }
        [[nodiscard]] bool IsRightAltDown() const { return (m_bits & kBitRightAlt) != 0; }
        [[nodiscard]] bool IsLeftCommandDown() const { return (m_bits & kBitLeftCommand) != 0; }
        [[nodiscard]] bool IsRightCommandDown() const { return (m_bits & kBitRightCommand) != 0; }

    private:
        static constexpr uint16_t kBitLeftShift = 1 << 0;
        static constexpr uint16_t kBitRightShift = 1 << 1;
        static constexpr uint16_t kBitLeftControl = 1 << 2;
        static constexpr uint16_t kBitRightControl = 1 << 3;
        static constexpr uint16_t kBitLeftAlt = 1 << 4;
        static constexpr uint16_t kBitRightAlt = 1 << 5;
        static constexpr uint16_t kBitLeftCommand = 1 << 6;
        static constexpr uint16_t kBitRightCommand = 1 << 7;
        static constexpr uint16_t kBitCapsLock = 1 << 8;

        uint16_t m_bits;
    };

} // namespace NS
