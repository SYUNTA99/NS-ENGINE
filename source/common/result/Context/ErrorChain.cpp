/// @file ErrorChain.cpp
/// @brief エラーチェーンの実装
#include "common/result/Context/ErrorChain.h"

#include <chrono>

namespace NS::result
{

    namespace
    {

        std::uint64_t GetCurrentTimestamp() noexcept
        {
            auto now = std::chrono::steady_clock::now();
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        }

    } // namespace

    ErrorChainBuilder::ErrorChainBuilder(::NS::Result result,
                                         SourceLocation location,
                                         std::string_view message) noexcept
    {
        m_chain.SetResult(
            ChainEntry{.result = result, .location = location, .message = message, .timestamp = GetCurrentTimestamp()});
    }

    ErrorChainBuilder& ErrorChainBuilder::CausedBy(::NS::Result cause,
                                                   SourceLocation location,
                                                   std::string_view message) noexcept
    {
        m_chain.AppendCause(
            ChainEntry{.result = cause, .location = location, .message = message, .timestamp = GetCurrentTimestamp()});
        return *this;
    }

    ErrorChainBuilder& ErrorChainBuilder::CausedBy(const ErrorChain& chain) noexcept
    {
        for (const auto& entry : chain.GetEntries())
        {
            m_chain.AppendCause(entry);
        }
        return *this;
    }

    ErrorChain ErrorChainBuilder::Build() const noexcept
    {
        return m_chain;
    }

    ::NS::Result ErrorChainBuilder::ToResult() noexcept
    {
        if (m_chain.GetDepth() > 0)
        {
            ::NS::Result result = m_chain.GetResult();
            detail::ChainStorage::Store(result, m_chain);
            return result;
        }
        return ::NS::Result{};
    }

    namespace detail
    {

        thread_local std::deque<ChainStorage::Entry> ChainStorage::s_storage;
        thread_local std::uint64_t ChainStorage::s_accessCounter = 0;

        void ChainStorage::Store(::NS::Result result, const ErrorChain& chain) noexcept
        {
            // 既存エントリを更新
            for (auto& entry : s_storage)
            {
                if (entry.result == result)
                {
                    entry.chain = chain;
                    entry.accessTime = ++s_accessCounter;
                    return;
                }
            }

            // サイズ超過時は古いエントリを削除
            if (s_storage.size() >= kMaxChains)
            {
                Evict();
            }

            // 新規追加
            s_storage.push_back(Entry{.result = result, .chain = chain, .accessTime = ++s_accessCounter});
        }

        std::optional<ErrorChain> ChainStorage::Find(::NS::Result result) noexcept
        {
            for (auto& entry : s_storage)
            {
                if (entry.result == result)
                {
                    entry.accessTime = ++s_accessCounter;
                    return entry.chain;
                }
            }
            return std::nullopt;
        }

        void ChainStorage::Remove(::NS::Result result) noexcept
        {
            auto it = std::find_if(
                s_storage.begin(), s_storage.end(), [&result](const Entry& e) { return e.result == result; });
            if (it != s_storage.end())
            {
                s_storage.erase(it);
            }
        }

        void ChainStorage::Clear() noexcept
        {
            s_storage.clear();
            s_accessCounter = 0;
        }

        std::size_t ChainStorage::GetCount() noexcept
        {
            return s_storage.size();
        }

        void ChainStorage::Evict() noexcept
        {
            if (s_storage.empty())
                return;

            auto minIt = std::min_element(s_storage.begin(), s_storage.end(), [](const Entry& a, const Entry& b) {
                return a.accessTime < b.accessTime;
            });

            if (minIt != s_storage.end())
            {
                s_storage.erase(minIt);
            }
        }

    } // namespace detail

} // namespace NS::result
