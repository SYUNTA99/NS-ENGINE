/// @file ResultContext.h
/// @brief コンテキスト付きResult型
#pragma once


#include "common/result/Context/SourceLocation.h"
#include "common/result/Core/Result.h"
#include "common/result/Core/ResultConfig.h"

#include <cstdint>
#include <deque>
#include <optional>
#include <string_view>
#include <vector>

namespace NS { namespace result {

    /// コンテキスト情報
    struct ContextInfo
    {
        SourceLocation location;
        std::string_view message;
        std::uint64_t timestamp = 0;

        [[nodiscard]] constexpr bool IsValid() const noexcept { return location.IsValid(); }
    };

    /// コンテキスト付きResult
    /// Resultを拡張し、発生箇所とメッセージを保持
    class ResultContext
    {
    public:
        ResultContext() noexcept = default;

        explicit ResultContext(::NS::Result result) noexcept : m_result(result) {}

        ResultContext(::NS::Result result, SourceLocation location, std::string_view message = {}) noexcept;

        /// 暗黙変換
        [[nodiscard]] constexpr operator ::NS::Result() const noexcept { return m_result; }

        /// Result委譲
        [[nodiscard]] constexpr bool IsSuccess() const noexcept { return m_result.IsSuccess(); }
        [[nodiscard]] constexpr bool IsFailure() const noexcept { return m_result.IsFailure(); }
        [[nodiscard]] constexpr int GetModule() const noexcept { return m_result.GetModule(); }
        [[nodiscard]] constexpr int GetDescription() const noexcept { return m_result.GetDescription(); }

        /// コンテキスト情報
        [[nodiscard]] const ContextInfo& GetContext() const noexcept { return m_context; }
        [[nodiscard]] bool HasContext() const noexcept { return m_context.IsValid(); }

        /// 生のResult取得
        [[nodiscard]] constexpr ::NS::Result GetResult() const noexcept { return m_result; }

    private:
        ::NS::Result m_result;
        ContextInfo m_context;
    };

    /// コンテキストストレージ（スレッドローカル）
    /// 設計方針:
    /// - LRUベースでサイズ制限（古いエントリから削除）
    /// - Result値 + タイムスタンプでユニーク識別
    /// - 同じResult値でも複数保持可能（異なるタイミングの発生）
    namespace detail
    {

        class ContextStorage
        {
        public:
            static constexpr std::size_t kMaxContexts = 64;
            static constexpr std::size_t kMaxPerResult = 4;

            /// コンテキストを記録
            static void Push(::NS::Result result, const ContextInfo& context) noexcept;

            /// 最新のコンテキストを取得して削除
            static std::optional<ContextInfo> Pop(::NS::Result result) noexcept;

            /// 最新のコンテキストを参照（削除しない）
            static std::optional<ContextInfo> Peek(::NS::Result result) noexcept;

            /// 特定Resultの全コンテキストを取得
            static std::vector<ContextInfo> GetAll(::NS::Result result) noexcept;

            /// クリア
            static void Clear() noexcept;

            /// 統計情報
            static std::size_t GetCount() noexcept;

        private:
            struct Entry
            {
                ::NS::Result result;
                ContextInfo context;
                std::uint64_t accessTime = 0;
            };

            static thread_local std::deque<Entry> s_storage;
            static thread_local std::uint64_t s_accessCounter;

            static void Evict() noexcept;
        };

    } // namespace detail

    /// コンテキストを記録
    inline void RecordContext(::NS::Result result, SourceLocation location, std::string_view message = {}) noexcept
    {
        if (result.IsFailure())
        {
            detail::ContextStorage::Push(result, ContextInfo{location, message, 0});
        }
    }

    /// コンテキストを取得
    [[nodiscard]] inline std::optional<ContextInfo> GetResultContext(::NS::Result result) noexcept
    {
        return detail::ContextStorage::Peek(result);
    }

    //=============================================================================
    // マクロ
    //=============================================================================

#if NS_RESULT_ENABLE_CONTEXT

/// 失敗時にコンテキストを記録して早期リターン
#define NS_RETURN_IF_FAILED_CTX(expr)                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto _result = (expr); _result.IsFailure())                                                                \
        {                                                                                                              \
            ::NS::result::RecordContext(_result, NS_CURRENT_SOURCE_LOCATION());                                        \
            return _result;                                                                                            \
        }                                                                                                              \
    }                                                                                                                  \
    while (false)

/// メッセージ付き
#define NS_RETURN_IF_FAILED_CTX_MSG(expr, msg)                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto _result = (expr); _result.IsFailure())                                                                \
        {                                                                                                              \
            ::NS::result::RecordContext(_result, NS_CURRENT_SOURCE_LOCATION(), msg);                                   \
            return _result;                                                                                            \
        }                                                                                                              \
    }                                                                                                                  \
    while (false)

/// コンテキスト付きResultを返す
#define NS_MAKE_RESULT_CTX(result) ::NS::result::ResultContext((result), NS_CURRENT_SOURCE_LOCATION())

#define NS_MAKE_RESULT_CTX_MSG(result, msg) ::NS::result::ResultContext((result), NS_CURRENT_SOURCE_LOCATION(), (msg))

#else

// リリースビルド: コンテキスト記録なし、単純なリターンのみ
#define NS_RETURN_IF_FAILED_CTX(expr) NS_RETURN_IF_FAILED(expr)
#define NS_RETURN_IF_FAILED_CTX_MSG(expr, msg) NS_RETURN_IF_FAILED(expr)
#define NS_MAKE_RESULT_CTX(result) (result)
#define NS_MAKE_RESULT_CTX_MSG(result, msg) (result)

#endif

}} // namespace NS::result
