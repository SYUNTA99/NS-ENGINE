//----------------------------------------------------------------------------
//! @file   cached_query.h
//! @brief  ECS CachedQuery - 再利用可能なキャッシュ付きクエリ
//----------------------------------------------------------------------------
#pragma once

#include "../actor.h"
#include "../archetype.h"
#include "query_cache.h"
#include <vector>
#include <cstdint>

namespace ECS {

// 前方宣言
class World;
class ArchetypeStorage;

//============================================================================
//! @brief CachedQuery
//!
//! 再利用可能なクエリオブジェクト。Archetypeマッチング結果をキャッシュし、
//! フレーム毎の再検索を回避する。
//!
//! @code
//! // 一度だけ作成（シーン初期化時など）
//! auto query = world.CreateCachedQuery<TransformData, SpriteData>();
//!
//! // 毎フレーム高速イテレーション
//! query.ForEach([](Actor e, TransformData& t, SpriteData& s) {
//!     // 処理
//! });
//! @endcode
//!
//! キャッシュ無効化:
//! - 新しいArchetypeが作成された時（自動検知）
//! - `Invalidate()` が呼ばれた時
//!
//! @tparam Ts コンポーネント型群
//============================================================================
template<typename... Ts>
class CachedQuery {
public:
    //------------------------------------------------------------------------
    //! @brief コンストラクタ（World::CreateCachedQuery()から呼び出し）
    //! @param world Worldへのポインタ
    //------------------------------------------------------------------------
    explicit CachedQuery(World* world) noexcept
        : world_(world), cacheVersion_(0) {}

    // コピー禁止、ムーブ許可
    CachedQuery(const CachedQuery&) = delete;
    CachedQuery& operator=(const CachedQuery&) = delete;
    CachedQuery(CachedQuery&&) = default;
    CachedQuery& operator=(CachedQuery&&) = default;

    //------------------------------------------------------------------------
    //! @brief マッチするActorに対してイテレーション
    //! @tparam Func 処理関数の型 void(Actor, Ts&...)
    //! @param func 各Actorに対して呼び出す関数
    //!
    //! @code
    //! query.ForEach([](Actor e, TransformData& t, SpriteData& s) {
    //!     t.position += velocity * dt;
    //! });
    //! @endcode
    //------------------------------------------------------------------------
    template<typename Func>
    void ForEach(Func&& func);

    //------------------------------------------------------------------------
    //! @brief マッチするActor数を取得
    //! @return マッチするActorの総数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Count();

    //------------------------------------------------------------------------
    //! @brief マッチするActorが存在するか確認
    //! @return 1つ以上存在すればtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool Any() { return Count() > 0; }

    //------------------------------------------------------------------------
    //! @brief マッチするActorが存在しないか確認
    //! @return 存在しなければtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool Empty() { return Count() == 0; }

    //------------------------------------------------------------------------
    //! @brief キャッシュを強制無効化
    //!
    //! 次回のForEach/Count呼び出しでキャッシュが再構築される。
    //------------------------------------------------------------------------
    void Invalidate() noexcept {
        cacheVersion_ = 0;
    }

    //------------------------------------------------------------------------
    //! @brief キャッシュが有効か確認
    //! @return 有効ならtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsCacheValid() const noexcept;

    //------------------------------------------------------------------------
    //! @brief キャッシュされたArchetype数を取得
    //! @return キャッシュ内のArchetype数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t CachedArchetypeCount() const noexcept {
        return cachedArchetypes_.size();
    }

private:
    //! @brief キャッシュを再構築
    void RebuildCache();

    World* world_;
    std::vector<Archetype*> cachedArchetypes_;  //!< マッチするArchetypeのキャッシュ
    uint32_t cacheVersion_;                      //!< 構築時のバージョン
};

} // namespace ECS
