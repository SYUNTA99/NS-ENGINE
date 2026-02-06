/// @file PreprocessorHelpers.h
/// @brief プリプロセッサヘルパーマクロ
/// @details プラットフォーム固有ヘッダの自動選択マクロを提供する。
#pragma once

// commonモジュールの基盤マクロを使用
#include "HAL/Platform.h"
#include "common/utility/macros.h"

// =============================================================================
// 文字列化・連結マクロ（HALエイリアス）
// =============================================================================

/// 文字列化マクロ
#define NS_STRINGIFY NS_MACRO_STRINGIZE

/// 文字列化マクロ（展開後に文字列化）
#define NS_STRINGIFY_MACRO(x) NS_MACRO_STRINGIZE(x)

/// トークン連結マクロ
#define NS_CONCAT NS_MACRO_CONCATENATE

/// トークン連結マクロ（展開後に連結）
#define NS_CONCAT_MACRO(a, b) NS_MACRO_CONCATENATE(a, b)

// =============================================================================
// プラットフォームヘッダ名
// =============================================================================

/// コンパイル対象プラットフォーム名
///
/// ビルドシステムから定義可能:
/// - premake: defines { "NS_COMPILED_PLATFORM=Windows" }
/// - cmake: add_definitions(-DNS_COMPILED_PLATFORM=Windows)
///
/// 未定義の場合はプラットフォームマクロから自動判定。
#if !defined(NS_COMPILED_PLATFORM)
#if PLATFORM_WINDOWS
#define NS_COMPILED_PLATFORM Windows
#elif PLATFORM_MAC
#define NS_COMPILED_PLATFORM Mac
#elif PLATFORM_LINUX
#define NS_COMPILED_PLATFORM Linux
#else
#error "NS_COMPILED_PLATFORM not defined and no supported platform detected"
#endif
#endif

/// プラットフォームヘッダ名（NS_COMPILED_PLATFORMのエイリアス）
#define PLATFORM_HEADER_NAME NS_COMPILED_PLATFORM

// =============================================================================
// COMPILED_PLATFORM_HEADER マクロ
// =============================================================================

/// @brief プラットフォーム固有ヘッダをインクルード
///
/// 使用例:
/// @code
/// #include COMPILED_PLATFORM_HEADER(PlatformMemory.h)
/// // Windows時: "Windows/WindowsPlatformMemory.h" に展開
/// // Mac時:     "Mac/MacPlatformMemory.h" に展開
/// // Linux時:   "Linux/LinuxPlatformMemory.h" に展開
/// @endcode
///
/// @param Suffix ヘッダファイル名（例: PlatformMemory.h）
/// @note マクロ展開の順序のため、内部ヘルパーマクロを使用

// 内部ヘルパー: プラットフォームごとにインクルードパスを生成
// 複雑なマクロ展開を避けるため、各プラットフォームで直接定義

// 文字列リテラル連結を使用してパスを生成
// "Windows/" "Windows" "PlatformTypes.h" → "Windows/WindowsPlatformTypes.h"
#if PLATFORM_WINDOWS
    #define COMPILED_PLATFORM_HEADER_DIR "Windows/"
    #define COMPILED_PLATFORM_HEADER_PREFIX "Windows"
#elif PLATFORM_MAC
    #define COMPILED_PLATFORM_HEADER_DIR "Mac/"
    #define COMPILED_PLATFORM_HEADER_PREFIX "Mac"
#elif PLATFORM_LINUX
    #define COMPILED_PLATFORM_HEADER_DIR "Linux/"
    #define COMPILED_PLATFORM_HEADER_PREFIX "Linux"
#else
    #error "No platform defined for COMPILED_PLATFORM_HEADER"
#endif

// マクロ引数を文字列化するヘルパー
#define COMPILED_PLATFORM_HEADER_STRINGIFY(x) #x
#define COMPILED_PLATFORM_HEADER_MAKE_PATH(prefix, suffix) COMPILED_PLATFORM_HEADER_STRINGIFY(prefix##suffix)
#define COMPILED_PLATFORM_HEADER_EXPAND(prefix, suffix) COMPILED_PLATFORM_HEADER_MAKE_PATH(prefix, suffix)

// 最終的なマクロ: #include COMPILED_PLATFORM_HEADER(PlatformTypes.h)
// Windows: "Windows/WindowsPlatformTypes.h" に展開
#define COMPILED_PLATFORM_HEADER(Suffix) COMPILED_PLATFORM_HEADER_DIR COMPILED_PLATFORM_HEADER_EXPAND(COMPILED_PLATFORM_HEADER_PREFIX, Suffix)

// =============================================================================
// プラットフォーム固有コード用マクロ
// =============================================================================

/// プラットフォーム固有コードの開始
/// @code
/// #if PLATFORM_WINDOWS
/// // Windows固有コード
/// #endif
/// @endcode

/// プラットフォーム固有ヘッダガード生成
#define PLATFORM_HEADER_GUARD(Name) NS_CONCAT_MACRO(NS_CONCAT_MACRO(NS_COMPILED_PLATFORM, _), Name)
