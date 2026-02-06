/// @file HAL.cpp
/// @brief HALモジュール共通実装
/// @details ヘッダの検証とモジュール初期化コードを含む。
#include "HAL/Platform.h"
#include "HAL/PlatformMacros.h"
#include "HAL/PlatformTypes.h"
#include "HAL/PreprocessorHelpers.h"

// コンパイル検証: マクロが正しく展開されることを確認
static_assert(PLATFORM_WINDOWS == 1 || PLATFORM_WINDOWS == 0, "PLATFORM_WINDOWS must be 0 or 1");
static_assert(PLATFORM_64BITS == 1 || PLATFORM_64BITS == 0, "PLATFORM_64BITS must be 0 or 1");

#if PLATFORM_WINDOWS
static_assert(PLATFORM_WINDOWS == 1, "PLATFORM_WINDOWS should be 1 on Windows");
#endif

// 型サイズ検証
static_assert(sizeof(NS::SIZE_T) == sizeof(::size), "SIZE_T size mismatch");
static_assert(sizeof(NS::UPTRINT) == sizeof(void*), "UPTRINT size mismatch");
static_assert(sizeof(NS::TCHAR) == sizeof(wchar_t), "TCHAR size mismatch");

namespace NS
{
    namespace HAL
    {
        // モジュール初期化（将来の拡張用）
        void Initialize()
        {
            // 現時点では何もしない
        }

        // モジュールシャットダウン（将来の拡張用）
        void Shutdown()
        {
            // 現時点では何もしない
        }
    } // namespace HAL
} // namespace NS
