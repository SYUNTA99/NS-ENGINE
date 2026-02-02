//----------------------------------------------------------------------------
//! @file   typed_query_impl.h
//! @brief  ECS TypedQuery 実装
//----------------------------------------------------------------------------
#pragma once

#include "../query/typed_query.h"
#include "../actor_registry.h"

namespace ECS {

//----------------------------------------------------------------------------
// ActorRegistry::Query 実装
//----------------------------------------------------------------------------
template<typename... AccessModes>
TypedQuery<AccessModes...> ActorRegistry::Query() {
    return TypedQuery<AccessModes...>(this);
}

//----------------------------------------------------------------------------
// TypedQuery::ForEach 実装
//----------------------------------------------------------------------------
template<typename... AccessModes>
template<typename Func>
void TypedQuery<AccessModes...>::ForEach(Func&& func) {
    // コンポーネント型を抽出
    using ComponentTypes = std::tuple<unwrap_access_t<AccessModes>...>;

    // フィルター条件を取得
    const auto& withTypes = this->GetWithTypes();
    const auto& withoutTypes = this->GetWithoutTypes();

    // マッチするArchetypeをイテレーション
    registry_->GetArchetypeStorage().ForEachMatching<unwrap_access_t<AccessModes>...>(
        [&func, &withTypes, &withoutTypes](Archetype& arch) {
            // With フィルター: 必須コンポーネントをすべて持っているか確認
            for (const auto& typeIdx : withTypes) {
                if (!arch.HasComponentByTypeIndex(typeIdx)) {
                    return;  // このArchetypeをスキップ
                }
            }

            // Without フィルター: 除外コンポーネントを持っていないか確認
            for (const auto& typeIdx : withoutTypes) {
                if (arch.HasComponentByTypeIndex(typeIdx)) {
                    return;  // このArchetypeをスキップ
                }
            }

            // オフセット配列を取得
            auto offsets = detail::GetOffsets<AccessModes...>(arch);

            // 各Chunkを処理
            const auto& metas = arch.GetChunkMetas();
            for (size_t ci = 0; ci < metas.size(); ++ci) {
                detail::ProcessChunk<AccessModes...>(arch, ci, offsets, func);
            }
        }
    );
}

} // namespace ECS
