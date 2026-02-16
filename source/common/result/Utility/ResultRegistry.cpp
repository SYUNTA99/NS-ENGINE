/// @file ResultRegistry.cpp
/// @brief エラー型レジストリの実装
#include "common/result/Utility/ResultRegistry.h"

#include <algorithm>

namespace NS { namespace result {

    namespace
    {

        std::uint32_t MakeKey(int module, int description) noexcept
        {
            return (static_cast<std::uint32_t>(module) << 16) | static_cast<std::uint32_t>(description);
        }

    } // namespace

    ResultRegistry& ResultRegistry::Instance() noexcept
    {
        static ResultRegistry instance;
        return instance;
    }

    void ResultRegistry::Register(const ResultTypeInfo& info) noexcept
    {
        // Freeze済みなら登録を無視
        if (m_frozen.load(std::memory_order_acquire))
        {
            return;
        }

        std::unique_lock lock(m_mutex);
        const auto key = MakeKey(info.module, info.description);
        m_types[key] = info;
        m_moduleTypes.emplace(info.module, info);
    }

    void ResultRegistry::FreezeAfterInit() noexcept
    {
        m_frozen.store(true, std::memory_order_release);
    }

    bool ResultRegistry::IsFrozen() const noexcept
    {
        return m_frozen.load(std::memory_order_acquire);
    }

    const ResultTypeInfo* ResultRegistry::Find(::NS::Result result) const noexcept
    {
        std::shared_lock lock(m_mutex);

        // 完全一致を検索
        const auto key = MakeKey(result.GetModule(), result.GetDescription());
        if (auto it = m_types.find(key); it != m_types.end())
        {
            return &it->second;
        }

        // 範囲型を検索
        const int module = result.GetModule();
        const int desc = result.GetDescription();
        auto range = m_moduleTypes.equal_range(module);
        for (auto it = range.first; it != range.second; ++it)
        {
            const auto& info = it->second;
            if (info.isRange && info.rangeBegin <= desc && desc < info.rangeEnd)
            {
                return &info;
            }
        }

        return nullptr;
    }

    std::vector<ResultTypeInfo> ResultRegistry::GetModuleTypes(int module) const noexcept
    {
        std::shared_lock lock(m_mutex);
        std::vector<ResultTypeInfo> result;

        auto range = m_moduleTypes.equal_range(module);
        for (auto it = range.first; it != range.second; ++it)
        {
            result.push_back(it->second);
        }

        std::sort(
            result.begin(), result.end(), [](const auto& a, const auto& b) { return a.description < b.description; });

        return result;
    }

    std::vector<ResultTypeInfo> ResultRegistry::GetAllTypes() const noexcept
    {
        std::shared_lock lock(m_mutex);
        std::vector<ResultTypeInfo> result;
        result.reserve(m_types.size());

        for (const auto& [key, info] : m_types)
        {
            result.push_back(info);
        }

        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            if (a.module != b.module)
                return a.module < b.module;
            return a.description < b.description;
        });

        return result;
    }

    std::size_t ResultRegistry::GetTypeCount() const noexcept
    {
        std::shared_lock lock(m_mutex);
        return m_types.size();
    }

    void ResultRegistry::Clear() noexcept
    {
        std::unique_lock lock(m_mutex);
        m_types.clear();
        m_moduleTypes.clear();
        m_frozen.store(false, std::memory_order_release);
    }

}} // namespace NS::result
