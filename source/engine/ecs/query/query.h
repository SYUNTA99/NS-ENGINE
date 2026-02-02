//----------------------------------------------------------------------------
//! @file   query.h
//! @brief  ECS Query API - 型安全なクエリビルダー
//----------------------------------------------------------------------------
#pragma once

#include "../access_mode.h"
#include "../actor.h"
#include "../archetype.h"
#include "../archetype_storage.h"
#include "query_types.h"
#include <functional>
#include <tuple>
#include <type_traits>

namespace ECS {

// 前方宣言
class World;

//============================================================================
//! @brief ChangeFilter - コンポーネント変更フィルター
//!
//! 指定バージョン以降に変更されたChunkのみを処理するためのフィルター情報。
//!
//! @tparam T フィルター対象のコンポーネント型
//============================================================================
template<typename T>
struct ChangeFilter {
    using Type = T;
    uint32_t sinceVersion = 0;  //!< このバージョンより新しいもののみ処理

    explicit ChangeFilter(uint32_t version = 0) noexcept : sinceVersion(version) {}
};

//============================================================================
//! @brief Query
//!
//! 型安全なクエリビルダー。指定したコンポーネントを持つActorを
//! 効率的にイテレーションする。
//!
//! 使用例:
//! ```cpp
//! // 基本的な使い方
//! world.Query<TransformData, SpriteData>().ForEach([](Actor e, auto& t, auto& s) {
//!     // 処理
//! });
//!
//! // Exclude - 特定コンポーネントを持たないEntityを検索
//! world.Query<TransformData>().Exclude<Dead>().ForEach([](Actor e, auto& t) {
//!     // Dead を持たない Entity のみ
//! });
//!
//! // Optional - あればT*、なければnullptr
//! world.Query<TransformData, Optional<VelocityData>>()
//!      .ForEach([](Actor e, TransformData& t, VelocityData* v) {
//!          if (v) { t.position += v->velocity * dt; }
//!      });
//!
//! // WithChangeFilter - 変更されたもののみ処理
//! world.Query<TransformData>()
//!      .WithChangeFilter<TransformData>(lastFrameVersion)
//!      .ForEach([](Actor e, auto& t) {
//!          // 前フレームから変更されたTransformDataのみ処理
//!      });
//! ```
//!
//! @tparam Ts コンポーネント型群（T, Optional<T>, Exclude<T> の組み合わせ）
//============================================================================
template<typename... Ts>
class Query {
public:
    //! @brief Worldからクエリを構築
    explicit Query(World* world) : world_(world) {}

    //------------------------------------------------------------------------
    //! @brief ラムダでイテレーション
    //!
    //! 指定コンポーネントを持つ全Actorに対して関数を実行する。
    //! - Required (T): T& として渡される
    //! - Optional<T>: T* として渡される（存在しなければnullptr）
    //! - Exclude<T>: 引数として渡されない（フィルタリングのみ）
    //!
    //! @tparam Func 処理関数の型
    //! @param func 各Actorに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename Func>
    void ForEach(Func&& func);

    //------------------------------------------------------------------------
    //! @brief 最初の1つを取得
    //! @return コンポーネントのタプル（見つからない場合は全てnullptr）
    //! @note Exclude<T>は結果に含まれない、Optional<T>はT*として含まれる
    //------------------------------------------------------------------------
    [[nodiscard]] auto First();

    //------------------------------------------------------------------------
    //! @brief 件数取得
    //! @return マッチするActor数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Count() const;

    //------------------------------------------------------------------------
    //! @brief Excludeフィルター追加
    //!
    //! 指定コンポーネントを持たないEntityのみマッチさせる。
    //!
    //! @tparam ExcludeTs 除外するコンポーネント型群
    //! @return 新しいQuery（Exclude<ExcludeTs>...を追加）
    //!
    //! 使用例:
    //! ```cpp
    //! world.Query<TransformData>().Exclude<Dead, Disabled>().ForEach(...);
    //! ```
    //------------------------------------------------------------------------
    template<typename... ExcludeTs>
    [[nodiscard]] Query<Ts..., Exclude<ExcludeTs>...> Exclude() {
        return Query<Ts..., ECS::Exclude<ExcludeTs>...>(world_);
    }

    //------------------------------------------------------------------------
    //! @brief 関数フィルター追加
    //! @param predicate 追加フィルター関数
    //! @return 自身への参照（チェーン可能）
    //------------------------------------------------------------------------
    Query& With(std::function<bool(Actor)> predicate) {
        predicates_.push_back(std::move(predicate));
        return *this;
    }

    //------------------------------------------------------------------------
    //! @brief 変更フィルター追加
    //!
    //! 指定バージョン以降に変更されたコンポーネントを持つChunkのみを処理。
    //!
    //! @tparam FilterT フィルター対象のコンポーネント型
    //! @param sinceVersion このバージョンより新しいもののみ処理
    //! @return 自身への参照（チェーン可能）
    //!
    //! 使用例:
    //! ```cpp
    //! uint32_t lastVersion = world.GetFrameCounter();
    //! // ... 次フレーム ...
    //! world.Query<TransformData>()
    //!      .WithChangeFilter<TransformData>(lastVersion)
    //!      .ForEach([](Actor e, auto& t) {
    //!          // lastVersion以降に変更されたもののみ
    //!      });
    //! ```
    //------------------------------------------------------------------------
    template<typename FilterT>
    Query& WithChangeFilter(uint32_t sinceVersion) {
        changeFilters_.emplace_back(
            std::type_index(typeid(FilterT)),
            sinceVersion
        );
        return *this;
    }

    //------------------------------------------------------------------------
    //! @brief 存在確認
    //! @return 1つ以上存在する場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool Any() const {
        return Count() > 0;
    }

    //------------------------------------------------------------------------
    //! @brief 存在しないか確認
    //! @return 1つも存在しない場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool Empty() const {
        return Count() == 0;
    }

    //------------------------------------------------------------------------
    //! @brief World取得（内部用）
    //------------------------------------------------------------------------
    [[nodiscard]] World* GetWorld() const noexcept { return world_; }

    //------------------------------------------------------------------------
    //! @brief フィルター取得（内部用）
    //------------------------------------------------------------------------
    [[nodiscard]] const std::vector<std::function<bool(Actor)>>& GetPredicates() const noexcept {
        return predicates_;
    }

    //------------------------------------------------------------------------
    //! @brief 変更フィルター取得（内部用）
    //! @return (type_index, sinceVersion)のペア配列
    //------------------------------------------------------------------------
    [[nodiscard]] const std::vector<std::pair<std::type_index, uint32_t>>& GetChangeFilters() const noexcept {
        return changeFilters_;
    }

private:
    //! @brief First()の実装ヘルパー（Exclude除外後の型で呼び出し）
    template<typename... PureTs>
    std::tuple<PureTs*...> FirstImpl(std::tuple<PureTs...>);

protected:
    //! @brief フィルターを適用
    [[nodiscard]] bool PassesFilters(Actor actor) const {
        for (const auto& pred : predicates_) {
            if (!pred(actor)) {
                return false;
            }
        }
        return true;
    }

    //! @brief Chunkが変更フィルターを通過するか
    //! @param arch Archetype
    //! @param chunkIndex Chunkインデックス
    //! @return 全ての変更フィルターを通過したらtrue
    [[nodiscard]] bool PassesChangeFilters(const Archetype& arch, size_t chunkIndex) const {
        for (const auto& [typeIdx, sinceVersion] : changeFilters_) {
            size_t compIdx = arch.GetComponentIndex(typeIdx);
            if (compIdx == SIZE_MAX) continue;  // コンポーネントがない場合はスキップ

            uint32_t chunkVersion = arch.GetComponentVersion(chunkIndex, compIdx);
            if (chunkVersion <= sinceVersion) {
                return false;  // バージョンが古いのでスキップ
            }
        }
        return true;
    }

    World* world_;
    std::vector<std::function<bool(Actor)>> predicates_;
    std::vector<std::pair<std::type_index, uint32_t>> changeFilters_;  //!< 変更フィルター
};

//============================================================================
//! @brief QueryBuilder
//!
//! Worldから呼び出されるヘルパー。Query<Ts...>を構築して返す。
//============================================================================
class QueryBuilder {
public:
    explicit QueryBuilder(World* world) : world_(world) {}

    template<typename... Ts>
    [[nodiscard]] Query<Ts...> Select() {
        return Query<Ts...>(world_);
    }

private:
    World* world_;
};

} // namespace ECS

// 実装はworld.hのインクルード後に必要（循環依存回避）
// query_impl.h で定義
