//----------------------------------------------------------------------------
//! @file   parallel_foreach_impl.h
//! @brief  World::ParallelForEach テンプレート実装
//! @note   このファイルはworld.hの末尾でインクルードされる（循環依存回避）
//----------------------------------------------------------------------------
#pragma once

#include "detail/foreach_helpers.h"
#include "typed_foreach.h"
#include "engine/core/job_system.h"
#include <array>
#include <vector>
#include <utility>

namespace ECS {

//----------------------------------------------------------------------------
//! @brief Chunk情報（並列処理用）
//----------------------------------------------------------------------------
struct ParallelChunkInfo {
    Archetype* arch;
    size_t chunkIndex;
};

//----------------------------------------------------------------------------
//! @brief 任意数のコンポーネントに対して並列イテレーション（SoA対応レガシー版）
//!
//! @tparam Ts コンポーネント型群（1つ以上）
//! @tparam Func 処理関数型 void(Actor, Ts&...)
//! @param func 各Actorとコンポーネントに対して呼び出す関数
//! @return JobHandle 完了待機用ハンドル
//!
//! @note Chunk単位で並列実行。ラムダ内でのActor生成/破棄は禁止。
//!       コンポーネント追加/削除はDeferred版を使用すること。
//----------------------------------------------------------------------------
template<typename... Ts, typename Func,
         std::enable_if_t<!detail::all_are_access_modes_v<Ts...>, int>>
JobHandle World::ParallelForEach(Func&& func) {
    static_assert(sizeof...(Ts) >= 1, "ParallelForEach requires at least one component type");

    // マッチするArchetypeのChunkを収集
    std::vector<ParallelChunkInfo> chunks;
    container_.ECS().GetArchetypeStorage().ForEachMatching<Ts...>([&chunks](Archetype& arch) {
        for (size_t ci = 0; ci < arch.GetChunkCount(); ++ci) {
            chunks.push_back({&arch, ci});
        }
    });

    if (chunks.empty()) {
        return JobHandle{};
    }

    // Chunk単位で並列実行
    // 注意: 引数の評価順序は未規定なので、size()をmove前に取得
    const uint32_t chunkCount = static_cast<uint32_t>(chunks.size());
    return JobSystem::Get().ParallelForRange(
        0, chunkCount,
        [chunks = std::move(chunks), func = std::forward<Func>(func)]
        (uint32_t begin, uint32_t end) {
#ifdef _DEBUG
            // 並列コンテキストフラグを立てる（構造変更を検知するため）
            ParallelContextGuard guard;
#endif
            for (uint32_t i = begin; i < end; ++i) {
                Archetype* arch = chunks[i].arch;
                size_t ci = chunks[i].chunkIndex;

                const uint16_t count = arch->GetChunkActorCount(ci);
                const Actor* actors = arch->GetActorArray(ci);

                // SoA: 各コンポーネント配列の先頭を取得
                std::array<std::byte*, sizeof...(Ts)> arrayBases = {
                    reinterpret_cast<std::byte*>(arch->GetComponentArray<Ts>(ci))...
                };

                for (uint16_t j = 0; j < count; ++j) {
                    detail::InvokeWithComponentsSoA<Ts...>(
                        func, actors[j], j, arrayBases,
                        std::index_sequence_for<Ts...>{}
                    );
                }
            }
        });
}

//----------------------------------------------------------------------------
//! @brief アクセスモード付き並列イテレーション（SoA対応型安全版）
//!
//! @tparam AccessModes In<T>/Out<T>/InOut<T>の組み合わせ
//! @tparam Func 処理関数型
//! @param func 各Actorとコンポーネントに対して呼び出す関数
//! @return JobHandle 完了待機用ハンドル
//!
//! コンパイル時チェック:
//! - 同じコンポーネント型への重複アクセスを検出
//! - ラムダ引数とアクセスモードの整合性を検証
//!
//! In<T>: const T& で渡される（読み取り専用）
//! Out<T>/InOut<T>: T& で渡される（書き込み可能）
//----------------------------------------------------------------------------
template<typename... AccessModes, typename Func,
         std::enable_if_t<detail::all_are_access_modes_v<AccessModes...>, int>>
JobHandle World::ParallelForEach(Func&& func) {
    static_assert(sizeof...(AccessModes) >= 1,
        "ParallelForEach requires at least one access mode");
    static_assert(sizeof...(AccessModes) <= 8,
        "ParallelForEach supports up to 8 components");

    // 全てがアクセスモードであることを検証
    static_assert(detail::all_are_access_modes_v<AccessModes...>,
        "All type parameters must be In<T>, Out<T>, or InOut<T>");

    // 同じコンポーネント型への重複アクセスを検出
    static_assert(!detail::has_duplicate_component_v<AccessModes...>,
        "Duplicate component access detected. Each component type can only appear once.");

    // ラムダの引数型がアクセスモードと一致することを検証
    static_assert(detail::validate_lambda_args_v<Func, AccessModes...>,
        "Lambda argument types must match access modes: "
        "In<T> requires const T&, Out<T>/InOut<T> requires T&");

    // マッチするArchetypeのChunkを収集
    std::vector<ParallelChunkInfo> chunks;
    container_.ECS().GetArchetypeStorage().ForEachMatching<unwrap_access_t<AccessModes>...>([&chunks](Archetype& arch) {
        for (size_t ci = 0; ci < arch.GetChunkCount(); ++ci) {
            chunks.push_back({&arch, ci});
        }
    });

    if (chunks.empty()) {
        return JobHandle{};
    }

    // Chunk単位で並列実行
    const uint32_t chunkCount = static_cast<uint32_t>(chunks.size());
    return JobSystem::Get().ParallelForRange(
        0, chunkCount,
        [chunks = std::move(chunks), func = std::forward<Func>(func)]
        (uint32_t begin, uint32_t end) {
#ifdef _DEBUG
            // 並列コンテキストフラグを立てる（構造変更を検知するため）
            ParallelContextGuard guard;
#endif
            for (uint32_t i = begin; i < end; ++i) {
                Archetype* arch = chunks[i].arch;
                size_t ci = chunks[i].chunkIndex;

                const uint16_t count = arch->GetChunkActorCount(ci);
                const Actor* actors = arch->GetActorArray(ci);

                // SoA: 各コンポーネント配列の先頭を取得
                std::array<std::byte*, sizeof...(AccessModes)> arrayBases = {
                    reinterpret_cast<std::byte*>(arch->GetComponentArray<unwrap_access_t<AccessModes>>(ci))...
                };

                for (uint16_t j = 0; j < count; ++j) {
                    detail::InvokeWithComponentsSoA<AccessModes...>(
                        func,
                        actors[j],
                        j,
                        arrayBases,
                        std::index_sequence_for<AccessModes...>{}
                    );
                }
            }
        });
}

} // namespace ECS
