/// @file ResultConfig.h
/// @brief Result型システムの設定
#pragma once

//=============================================================================
// 診断機能の有効/無効
//=============================================================================

/// NS_RESULT_ENABLE_DIAGNOSTICS を定義すると診断機能が有効になる
/// - コンテキスト記録 (SourceLocation)
/// - エラーチェーン
/// - 統計収集
/// - ログ出力
///
/// 未定義の場合、これらの機能は空の処理になる（オーバーヘッドなし）
///
/// 推奨設定:
/// - Debug: 定義する
/// - Release: 定義しない（または明示的に undef）

#if defined(_DEBUG) || defined(NS_RESULT_FORCE_DIAGNOSTICS)
#define NS_RESULT_ENABLE_DIAGNOSTICS 1
#else
#define NS_RESULT_ENABLE_DIAGNOSTICS 0
#endif

//=============================================================================
// 個別機能の有効/無効（細かい制御が必要な場合）
//=============================================================================

#ifndef NS_RESULT_ENABLE_CONTEXT
#define NS_RESULT_ENABLE_CONTEXT NS_RESULT_ENABLE_DIAGNOSTICS
#endif

#ifndef NS_RESULT_ENABLE_CHAIN
#define NS_RESULT_ENABLE_CHAIN NS_RESULT_ENABLE_DIAGNOSTICS
#endif

#ifndef NS_RESULT_ENABLE_STATISTICS
#define NS_RESULT_ENABLE_STATISTICS NS_RESULT_ENABLE_DIAGNOSTICS
#endif

#ifndef NS_RESULT_ENABLE_LOGGING
#define NS_RESULT_ENABLE_LOGGING NS_RESULT_ENABLE_DIAGNOSTICS
#endif
