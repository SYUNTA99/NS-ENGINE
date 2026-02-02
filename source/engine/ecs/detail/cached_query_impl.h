//----------------------------------------------------------------------------
//! @file   cached_query_impl.h
//! @brief  CachedQuery テンプレート実装
//!
//! @note このファイルはworld.h末尾でインクルードされる。
//!       Worldの完全な定義が必要なため、単体でインクルードしないこと。
//----------------------------------------------------------------------------
#pragma once

#include "../query/cached_query.h"
#include "../world.h"
#include "../archetype_storage.h"

namespace ECS {

template<typename... Ts>
bool CachedQuery<Ts...>::IsCacheValid() const noexcept {
    return cacheVersion_ == world_->GetArchetypeStorage().GetQueryCache().GetVersion();
}

template<typename... Ts>
void CachedQuery<Ts...>::RebuildCache() {
    cachedArchetypes_.clear();

    ArchetypeStorage& storage = world_->GetArchetypeStorage();

    // マッチするArchetypeを収集
    storage.ForEachMatchingFiltered<Ts...>([this](Archetype& arch) {
        cachedArchetypes_.push_back(&arch);
    });

    // 現在のバージョンを記録
    cacheVersion_ = storage.GetQueryCache().GetVersion();
}

template<typename... Ts>
size_t CachedQuery<Ts...>::Count() {
    if (!IsCacheValid()) {
        RebuildCache();
    }

    size_t count = 0;
    for (Archetype* arch : cachedArchetypes_) {
        count += arch->GetActorCount();
    }
    return count;
}

//----------------------------------------------------------------------------
// ヘルパー: オフセット取得とコンポーネント参照取得
//----------------------------------------------------------------------------
namespace detail {

template<typename T>
T& GetComponentRef(Archetype& arch, std::byte* compBase, uint16_t index, size_t compDataSize) {
    size_t offset = arch.GetComponentOffset<T>();
    return *reinterpret_cast<T*>(compBase + index * compDataSize + offset);
}

} // namespace detail

template<typename... Ts>
template<typename Func>
void CachedQuery<Ts...>::ForEach(Func&& func) {
    // キャッシュが無効なら再構築
    if (!IsCacheValid()) {
        RebuildCache();
    }

    // キャッシュされたArchetypeをイテレーション
    for (Archetype* arch : cachedArchetypes_) {
        const auto& metas = arch->GetChunkMetas();
        size_t compDataSize = arch->GetComponentDataSize();

        for (size_t ci = 0; ci < metas.size(); ++ci) {
            const Actor* actors = arch->GetActorArray(ci);
            std::byte* compBase = arch->GetComponentDataBase(ci);

            for (uint16_t i = 0; i < metas[ci].count; ++i) {
                // fold expressionで各コンポーネント参照を取得して呼び出し
                func(actors[i],
                     detail::GetComponentRef<Ts>(*arch, compBase, i, compDataSize)...);
            }
        }
    }
}

} // namespace ECS
