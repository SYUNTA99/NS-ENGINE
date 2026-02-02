//----------------------------------------------------------------------------
//! @file   query_impl.h
//! @brief  ECS Query - テンプレート実装
//!
//! query.h と world.h の循環依存を解決するための実装ファイル。
//! world.h をインクルードした後にこのファイルをインクルードすること。
//----------------------------------------------------------------------------
#pragma once

#include "../query/query.h"
#include "../world.h"
#include "foreach_helpers.h"
#include <array>

namespace ECS {

namespace detail {

//============================================================================
// Query用ヘルパー: tupleにポインタを設定（SoA版）
//============================================================================
template<typename... Ts, size_t... Is>
void SetTuplePointersSoA(
    std::tuple<Ts*...>& result,
    const std::array<std::byte*, sizeof...(Ts)>& arrayBases,
    uint16_t entityIndex,
    std::index_sequence<Is...>)
{
    ((std::get<Is>(result) = &reinterpret_cast<Ts*>(arrayBases[Is])[entityIndex]), ...);
}

//============================================================================
// Chunk変更フィルター判定
//============================================================================
inline bool PassesChangeFiltersImpl(
    const Archetype& arch,
    size_t chunkIndex,
    const std::vector<std::pair<std::type_index, uint32_t>>& changeFilters)
{
    for (const auto& [typeIdx, sinceVersion] : changeFilters) {
        size_t compIdx = arch.GetComponentIndex(typeIdx);
        if (compIdx == SIZE_MAX) continue;  // コンポーネントがない場合はスキップ

        uint32_t chunkVersion = arch.GetComponentVersion(chunkIndex, compIdx);
        if (chunkVersion <= sinceVersion) {
            return false;  // バージョンが古いのでスキップ
        }
    }
    return true;
}

//============================================================================
// 純粋なRequired型のみでForEachを実行するヘルパー（SoA版）
//============================================================================
template<typename... PureTs, typename Func>
void ForEachPureRequired(
    Archetype& arch,
    Func&& func,
    const std::vector<std::function<bool(Actor)>>& predicates,
    const std::vector<std::pair<std::type_index, uint32_t>>& changeFilters = {})
{
    const auto& metas = arch.GetChunkMetas();

    for (size_t ci = 0; ci < metas.size(); ++ci) {
        // 変更フィルターチェック（Chunk単位）
        if (!changeFilters.empty() && !PassesChangeFiltersImpl(arch, ci, changeFilters)) {
            continue;
        }

        const Actor* actors = arch.GetActorArray(ci);
        const uint16_t count = metas[ci].count;

        // SoA: 各コンポーネント配列の先頭を取得
        std::array<std::byte*, sizeof...(PureTs)> arrayBases = {
            reinterpret_cast<std::byte*>(arch.GetComponentArray<PureTs>(ci))...
        };

        for (uint16_t i = 0; i < count; ++i) {
            // predicateフィルターチェック
            bool pass = true;
            for (const auto& pred : predicates) {
                if (!pred(actors[i])) {
                    pass = false;
                    break;
                }
            }
            if (pass) {
                InvokeWithComponentsSoA<PureTs...>(
                    func, actors[i], i, arrayBases,
                    std::index_sequence_for<PureTs...>{}
                );
            }
        }
    }
}

//============================================================================
// タプルからForEachPureRequiredを呼び出すディスパッチャー
//============================================================================
template<typename Tuple, typename Func>
struct ForEachDispatcher;

template<typename... PureTs, typename Func>
struct ForEachDispatcher<std::tuple<PureTs...>, Func> {
    static void Execute(
        Archetype& arch,
        Func&& func,
        const std::vector<std::function<bool(Actor)>>& predicates,
        const std::vector<std::pair<std::type_index, uint32_t>>& changeFilters = {})
    {
        ForEachPureRequired<PureTs...>(arch, std::forward<Func>(func), predicates, changeFilters);
    }
};

} // namespace detail

//----------------------------------------------------------------------------
// Query::ForEach - 任意数のコンポーネントに対応（Exclude対応）
//----------------------------------------------------------------------------
template<typename... Ts>
template<typename Func>
void Query<Ts...>::ForEach(Func&& func) {
    static_assert(sizeof...(Ts) >= 1, "Query requires at least one component type");

    // 純粋なRequired型を抽出（Optional, Excludeを除く）
    using PureRequired = filter_pure_required_t<Ts...>;

    // Excludeがない場合は最適化パスを使用
    constexpr bool hasExclude = (is_exclude_v<Ts> || ...);

    if constexpr (!hasExclude) {
        // Excludeなし: 既存の高速パス
        if (predicates_.empty() && changeFilters_.empty()) {
            world_->template ForEach<Ts...>(std::forward<Func>(func));
            return;
        }

        // フィルターがある場合はArchetypeを直接走査
        world_->GetArchetypeStorage().ForEachMatching<Ts...>([this, &func](Archetype& arch) {
            const auto& metas = arch.GetChunkMetas();

            for (size_t ci = 0; ci < metas.size(); ++ci) {
                // 変更フィルターチェック（Chunk単位）
                if (!changeFilters_.empty() &&
                    !detail::PassesChangeFiltersImpl(arch, ci, changeFilters_)) {
                    continue;
                }

                const Actor* actors = arch.GetActorArray(ci);
                const uint16_t count = metas[ci].count;

                // SoA: 各コンポーネント配列の先頭を取得
                std::array<std::byte*, sizeof...(Ts)> arrayBases = {
                    reinterpret_cast<std::byte*>(arch.GetComponentArray<Ts>(ci))...
                };

                for (uint16_t i = 0; i < count; ++i) {
                    if (PassesFilters(actors[i])) {
                        detail::InvokeWithComponentsSoA<Ts...>(
                            func, actors[i], i, arrayBases,
                            std::index_sequence_for<Ts...>{}
                        );
                    }
                }
            }
        });
    } else {
        // Excludeあり: ForEachMatchingFiltered使用
        world_->GetArchetypeStorage().ForEachMatchingFiltered<Ts...>(
            [this, &func](Archetype& arch) {
                detail::ForEachDispatcher<PureRequired, Func>::Execute(
                    arch, std::forward<Func>(func), predicates_, changeFilters_
                );
            });
    }
}

//----------------------------------------------------------------------------
// Query::First - 最初のマッチを取得（Exclude対応）
//----------------------------------------------------------------------------
template<typename... Ts>
auto Query<Ts...>::First() {
    static_assert(sizeof...(Ts) >= 1, "Query requires at least one component type");

    // 純粋なRequired型を抽出
    using PureRequired = filter_pure_required_t<Ts...>;

    // 戻り値型: 純粋なRequired型のポインタタプル
    return FirstImpl(PureRequired{});
}

// First実装ヘルパー（SoA版）
template<typename... Ts>
template<typename... PureTs>
std::tuple<PureTs*...> Query<Ts...>::FirstImpl(std::tuple<PureTs...>) {
    std::tuple<PureTs*...> result{};
    bool found = false;

    world_->GetArchetypeStorage().ForEachMatchingFiltered<Ts...>(
        [this, &result, &found](Archetype& arch) {
            if (found) return;

            const auto& metas = arch.GetChunkMetas();

            for (size_t ci = 0; ci < metas.size() && !found; ++ci) {
                // 変更フィルターチェック（Chunk単位）
                if (!changeFilters_.empty() &&
                    !detail::PassesChangeFiltersImpl(arch, ci, changeFilters_)) {
                    continue;
                }

                const Actor* actors = arch.GetActorArray(ci);
                const uint16_t count = metas[ci].count;

                // SoA: 各コンポーネント配列の先頭を取得
                std::array<std::byte*, sizeof...(PureTs)> arrayBases = {
                    reinterpret_cast<std::byte*>(arch.GetComponentArray<PureTs>(ci))...
                };

                for (uint16_t i = 0; i < count && !found; ++i) {
                    if (PassesFilters(actors[i])) {
                        detail::SetTuplePointersSoA<PureTs...>(
                            result, arrayBases, i,
                            std::index_sequence_for<PureTs...>{}
                        );
                        found = true;
                    }
                }
            }
        });

    return result;
}

//----------------------------------------------------------------------------
// Query::Count - マッチ数をカウント（Exclude対応）
//----------------------------------------------------------------------------
template<typename... Ts>
size_t Query<Ts...>::Count() const {
    static_assert(sizeof...(Ts) >= 1, "Query requires at least one component type");

    size_t count = 0;

    // ForEachMatchingFilteredを使用（Exclude対応）
    // const_castが必要（ForEachMatchingFilteredがnon-constのため）
    auto& storage = const_cast<ArchetypeStorage&>(world_->GetArchetypeStorage());

    if (predicates_.empty() && changeFilters_.empty()) {
        // フィルターがない場合はArchetypeのActor数を合計
        storage.ForEachMatchingFiltered<Ts...>([&count](Archetype& arch) {
            count += arch.GetActorCount();
        });
    } else {
        // フィルターがある場合は個別にチェック
        storage.ForEachMatchingFiltered<Ts...>([this, &count](Archetype& arch) {
            const auto& metas = arch.GetChunkMetas();

            for (size_t ci = 0; ci < metas.size(); ++ci) {
                // 変更フィルターチェック（Chunk単位）
                if (!changeFilters_.empty() &&
                    !detail::PassesChangeFiltersImpl(arch, ci, changeFilters_)) {
                    continue;
                }

                const Actor* actors = arch.GetActorArray(ci);
                const uint16_t actorCount = metas[ci].count;

                for (uint16_t i = 0; i < actorCount; ++i) {
                    if (PassesFilters(actors[i])) {
                        ++count;
                    }
                }
            }
        });
    }

    return count;
}

} // namespace ECS
