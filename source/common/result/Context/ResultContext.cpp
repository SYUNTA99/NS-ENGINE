/// @file ResultContext.cpp
/// @brief コンテキスト付きResult型の実装
#include "common/result/Context/ResultContext.h"

#include <algorithm>
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

    ResultContext::ResultContext(::NS::Result result, SourceLocation location, std::string_view message) noexcept
        : m_result(result)
    {
        m_context = ContextInfo{.location = location, .message = message, .timestamp = GetCurrentTimestamp()};

        if (result.IsFailure())
        {
            detail::ContextStorage::Push(result, m_context);
        }
    }

    namespace detail
    {

        thread_local std::deque<ContextStorage::Entry> ContextStorage::s_storage;
        thread_local std::uint64_t ContextStorage::s_accessCounter = 0;

        void ContextStorage::Push(::NS::Result result, const ContextInfo& context) noexcept
        {
            // 同一Resultの保持数制限をチェック
            std::size_t sameResultCount = static_cast<std::size_t>(std::count_if(
                s_storage.begin(), s_storage.end(), [&result](const Entry& e) { return e.result == result; }));

            if (sameResultCount >= kMaxPerResult)
            {
                // 同一Resultの最も古いエントリを削除
                auto it = std::find_if(
                    s_storage.begin(), s_storage.end(), [&result](const Entry& e) { return e.result == result; });
                if (it != s_storage.end())
                {
                    s_storage.erase(it);
                }
            }

            // サイズ超過時は古いエントリを削除
            if (s_storage.size() >= kMaxContexts)
            {
                Evict();
            }

            s_storage.push_back(Entry{.result = result, .context = context, .accessTime = ++s_accessCounter});
        }

        std::optional<ContextInfo> ContextStorage::Pop(::NS::Result result) noexcept
        {
            // 後ろから検索（最新のものを返す）
            for (auto it = s_storage.rbegin(); it != s_storage.rend(); ++it)
            {
                if (it->result == result)
                {
                    ContextInfo info = it->context;
                    s_storage.erase(std::next(it).base());
                    return info;
                }
            }
            return std::nullopt;
        }

        std::optional<ContextInfo> ContextStorage::Peek(::NS::Result result) noexcept
        {
            // 後ろから検索（最新のものを返す）
            for (auto it = s_storage.rbegin(); it != s_storage.rend(); ++it)
            {
                if (it->result == result)
                {
                    it->accessTime = ++s_accessCounter;
                    return it->context;
                }
            }
            return std::nullopt;
        }

        std::vector<ContextInfo> ContextStorage::GetAll(::NS::Result result) noexcept
        {
            std::vector<ContextInfo> results;
            for (const auto& entry : s_storage)
            {
                if (entry.result == result)
                {
                    results.push_back(entry.context);
                }
            }
            return results;
        }

        void ContextStorage::Clear() noexcept
        {
            s_storage.clear();
            s_accessCounter = 0;
        }

        std::size_t ContextStorage::GetCount() noexcept
        {
            return s_storage.size();
        }

        void ContextStorage::Evict() noexcept
        {
            if (s_storage.empty())
                return;

            // LRU: accessTimeが最小のエントリを削除
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
