/// @file ErrorChain.h
/// @brief エラーチェーン（原因追跡）
#pragma once

#include "common/result/Context/SourceLocation.h"
#include "common/result/Core/Result.h"
#include "common/result/Core/ResultConfig.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace NS::result
{

    /// チェーン内のエラーエントリ
    struct ChainEntry
    {
        ::NS::Result result;
        SourceLocation location;
        std::string_view message;
        std::uint64_t timestamp = 0;

        [[nodiscard]] constexpr bool IsValid() const noexcept { return result.IsFailure(); }
    };

    /// エラーチェーン（固定サイズ）
    /// チェーンの順序:
    ///   [0] = 最終エラー（最上位、呼び出し元に返されるエラー）
    ///   [1] = 直接の原因
    ///   [2] = その原因
    ///   ...
    ///   [N-1] = 根本原因（最も深いエラー）
    ///
    /// 例: Initialize() -> LoadConfig() -> ReadFile() がエラー
    ///   [0] = InitializationFailed (Initialize で返す)
    ///   [1] = LoadFailed (LoadConfig で返す)
    ///   [2] = PathNotFound (ReadFile の結果、根本原因)
    class ErrorChain
    {
    public:
        static constexpr std::size_t kMaxDepth = 8;

        ErrorChain() noexcept = default;

        /// 最終エラー（チェーンの先頭、呼び出し元に返される）
        [[nodiscard]] ::NS::Result GetResult() const noexcept
        {
            return m_count > 0 ? m_entries[0].result : ::NS::Result{};
        }

        /// 根本原因（チェーンの末尾、最も深いエラー）
        [[nodiscard]] ::NS::Result GetRootCause() const noexcept
        {
            return m_count > 0 ? m_entries[m_count - 1].result : ::NS::Result{};
        }

        /// エントリ数
        [[nodiscard]] std::size_t GetDepth() const noexcept { return m_count; }

        /// 空かどうか
        [[nodiscard]] bool IsEmpty() const noexcept { return m_count == 0; }

        /// エントリ取得（0=最終エラー, N-1=根本原因）
        [[nodiscard]] const ChainEntry& operator[](std::size_t index) const noexcept { return m_entries[index]; }

        /// イテレーション（最終エラー→根本原因の順）
        [[nodiscard]] std::span<const ChainEntry> GetEntries() const noexcept
        {
            return std::span(m_entries.data(), m_count);
        }

        /// コピーしてエントリを取得（span寿命問題を回避）
        [[nodiscard]] std::vector<ChainEntry> GetEntriesCopy() const noexcept
        {
            return std::vector<ChainEntry>(m_entries.begin(), m_entries.begin() + m_count);
        }

    private:
        friend class ErrorChainBuilder;

        std::array<ChainEntry, kMaxDepth> m_entries{};
        std::size_t m_count = 0;

        /// 末尾に追加（内部用、原因を追加）
        void AppendCause(const ChainEntry& entry) noexcept
        {
            if (m_count < kMaxDepth)
            {
                m_entries[m_count++] = entry;
            }
        }

        /// 先頭に設定（内部用、最終エラーを設定）
        void SetResult(const ChainEntry& entry) noexcept
        {
            if (m_count == 0)
            {
                m_entries[m_count++] = entry;
            }
            else
            {
                m_entries[0] = entry;
            }
        }
    };

    /// チェーンビルダー
    /// 使用例:
    ///   // ReadFile() が PathNotFound を返した場合
    ///   return ErrorChainBuilder(ResultLoadFailed(), location, "Config load failed")
    ///       .CausedBy(readFileResult)  // PathNotFound が根本原因
    ///       .ToResult();
    ///
    ///   // チェーン: [0]=LoadFailed, [1]=PathNotFound
    class ErrorChainBuilder
    {
    public:
        /// 最終エラー（呼び出し元に返すエラー）を指定してビルダーを作成
        explicit ErrorChainBuilder(::NS::Result result,
                                   SourceLocation location = SourceLocation::FromStd(),
                                   std::string_view message = {}) noexcept;

        /// 原因エラーを追加（チェーンの末尾に追加される）
        /// 複数回呼び出すと、呼び出し順にチェーンが深くなる
        ErrorChainBuilder& CausedBy(::NS::Result cause,
                                    SourceLocation location = {},
                                    std::string_view message = {}) noexcept;

        /// 別のチェーンを原因として追加（チェーン全体を末尾に追加）
        ErrorChainBuilder& CausedBy(const ErrorChain& chain) noexcept;

        /// チェーンを構築（ストレージには保存しない）
        [[nodiscard]] ErrorChain Build() const noexcept;

        /// Resultとして取得し、チェーンをストレージに保存
        [[nodiscard]] ::NS::Result ToResult() noexcept;

    private:
        ErrorChain m_chain;
    };

    /// チェーンストレージ（スレッドローカル）
    /// 設計方針:
    /// - LRUベースでサイズ制限
    /// - ErrorChainをコピー保持（span寿命問題を回避）
    namespace detail
    {

        class ChainStorage
        {
        public:
            static constexpr std::size_t kMaxChains = 32;

            /// チェーンを保存
            static void Store(::NS::Result result, const ErrorChain& chain) noexcept;

            /// チェーンを検索（コピーを返す、寿命問題を回避）
            static std::optional<ErrorChain> Find(::NS::Result result) noexcept;

            /// チェーンを削除
            static void Remove(::NS::Result result) noexcept;

            /// クリア
            static void Clear() noexcept;

            /// 統計
            static std::size_t GetCount() noexcept;

        private:
            struct Entry
            {
                ::NS::Result result;
                ErrorChain chain;
                std::uint64_t accessTime = 0;
            };

            static thread_local std::deque<Entry> s_storage;
            static thread_local std::uint64_t s_accessCounter;

            static void Evict() noexcept;
        };

    } // namespace detail

#if NS_RESULT_ENABLE_CHAIN

    /// チェーン付きResultを作成
    [[nodiscard]] inline ::NS::Result MakeChainedResult(::NS::Result result,
                                                        ::NS::Result cause,
                                                        std::string_view message = {},
                                                        SourceLocation location = SourceLocation::FromStd()) noexcept
    {
        ErrorChainBuilder builder(result, location, message);
        builder.CausedBy(cause);
        return builder.ToResult();
    }

    /// エラーチェーンを取得（コピーを返す）
    [[nodiscard]] inline std::optional<ErrorChain> GetErrorChain(::NS::Result result) noexcept
    {
        return detail::ChainStorage::Find(result);
    }

    /// エラーチェーンがあるか
    [[nodiscard]] inline bool HasErrorChain(::NS::Result result) noexcept
    {
        return GetErrorChain(result).has_value();
    }

#else

    // リリースビルド: チェーンなし、単純にresultを返す
    [[nodiscard]] inline ::NS::Result MakeChainedResult(::NS::Result result,
                                                        ::NS::Result /*cause*/,
                                                        std::string_view /*message*/ = {},
                                                        SourceLocation /*location*/ = {}) noexcept
    {
        return result;
    }

    [[nodiscard]] inline std::optional<ErrorChain> GetErrorChain(::NS::Result /*result*/) noexcept
    {
        return std::nullopt;
    }

    [[nodiscard]] inline bool HasErrorChain(::NS::Result /*result*/) noexcept
    {
        return false;
    }

#endif

} // namespace NS::result
