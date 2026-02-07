/// @file ResultRegistry.h
/// @brief エラー型レジストリ
#pragma once


#include "common/result/Core/Result.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace NS::result
{

    /// エラー型情報
    struct ResultTypeInfo
    {
        int module = 0;
        int description = 0;
        std::string_view typeName;
        std::string_view fullName;
        std::string_view message;
        bool isRange = false;
        int rangeBegin = 0;
        int rangeEnd = 0;
    };

    /// エラー型レジストリ
    /// スレッドセーフティ:
    /// - 起動時（main前）の登録はシングルスレッドで行われると仮定
    /// - 実行時は読み取りのみ（書き込みロック不要）
    /// - FreezeAfterInit()呼び出し後は完全に読み取り専用
    class ResultRegistry
    {
    public:
        /// シングルトン取得
        [[nodiscard]] static ResultRegistry& Instance() noexcept;

        /// 型を登録（起動時のみ呼び出し可能）
        /// FreezeAfterInit()呼び出し後は無視される
        void Register(const ResultTypeInfo& info) noexcept;

        /// 初期化完了を宣言（以降の登録を禁止）
        /// main()開始時に呼び出すことを推奨
        void FreezeAfterInit() noexcept;

        /// 初期化完了済みか
        [[nodiscard]] bool IsFrozen() const noexcept;

        /// Resultから型情報を取得
        [[nodiscard]] const ResultTypeInfo* Find(::NS::Result result) const noexcept;

        /// モジュールの全型を取得
        [[nodiscard]] std::vector<ResultTypeInfo> GetModuleTypes(int module) const noexcept;

        /// 全型を取得
        [[nodiscard]] std::vector<ResultTypeInfo> GetAllTypes() const noexcept;

        /// 型数を取得
        [[nodiscard]] std::size_t GetTypeCount() const noexcept;

        /// 登録をクリア（テスト用、Freeze解除も行う）
        void Clear() noexcept;

    private:
        ResultRegistry() = default;

        std::atomic<bool> m_frozen{false};
        mutable std::shared_mutex m_mutex;
        std::unordered_map<std::uint32_t, ResultTypeInfo> m_types;
        std::unordered_multimap<int, ResultTypeInfo> m_moduleTypes;
    };

    /// 自動登録用ヘルパー
    template <typename ResultType> class ResultAutoRegister
    {
    public:
        ResultAutoRegister(std::string_view typeName, std::string_view fullName, std::string_view message) noexcept
        {
            ResultTypeInfo info{.module = ResultType::kModule,
                                .description = ResultType::kDescription,
                                .typeName = typeName,
                                .fullName = fullName,
                                .message = message};
            ResultRegistry::Instance().Register(info);
        }
    };

    /// 範囲型の自動登録用
    template <typename ResultType> class ResultRangeAutoRegister
    {
    public:
        ResultRangeAutoRegister(std::string_view typeName, std::string_view fullName, std::string_view message) noexcept
        {
            ResultTypeInfo info{.module = ResultType::kModule,
                                .description = ResultType::kDescriptionBegin,
                                .typeName = typeName,
                                .fullName = fullName,
                                .message = message,
                                .isRange = true,
                                .rangeBegin = ResultType::kDescriptionBegin,
                                .rangeEnd = ResultType::kDescriptionEnd};
            ResultRegistry::Instance().Register(info);
        }
    };

} // namespace NS::result

/// 自動登録マクロ
#define NS_REGISTER_RESULT(ResultType, message)                                                                        \
    static ::NS::result::ResultAutoRegister<ResultType> _ns_result_register_##ResultType(                              \
        #ResultType, #ResultType, message)

#define NS_REGISTER_RESULT_RANGE(ResultType, message)                                                                  \
    static ::NS::result::ResultRangeAutoRegister<ResultType> _ns_result_register_##ResultType(                         \
        #ResultType, #ResultType, message)
