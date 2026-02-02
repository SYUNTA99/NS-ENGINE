//----------------------------------------------------------------------------
//! @file   typed_query.h
//! @brief  ECS TypedQuery - In/Out/InOut対応の型安全クエリ
//----------------------------------------------------------------------------
#pragma once

#include "../access_mode.h"
#include "../actor.h"
#include "../archetype.h"
#include "../archetype_storage.h"
#include <type_traits>
#include <tuple>
#include <vector>
#include <typeindex>

namespace ECS {

// 前方宣言
class ActorRegistry;

namespace detail {

//============================================================================
// ForEach実装ヘルパー
//============================================================================

// インデックスシーケンスを使ってコンポーネントを展開
template<typename... AccessModes, typename Func, size_t... Is>
void InvokeWithComponents(
    Actor actor,
    std::byte* compBase,
    size_t compDataSize,
    const std::array<size_t, sizeof...(AccessModes)>& offsets,
    Func&& func,
    std::index_sequence<Is...>)
{
    // 各アクセスモードに対応する型でコンポーネントを取得
    func(
        actor,
        *reinterpret_cast<unwrap_access_t<AccessModes>*>(
            compBase + offsets[Is]
        )...
    );
}

// Chunk内の全Actorを処理
template<typename... AccessModes, typename Func>
void ProcessChunk(
    Archetype& arch,
    size_t chunkIndex,
    const std::array<size_t, sizeof...(AccessModes)>& offsets,
    Func&& func)
{
    const auto& metas = arch.GetChunkMetas();
    const Actor* actors = arch.GetActorArray(chunkIndex);
    std::byte* compBase = arch.GetComponentDataBase(chunkIndex);
    size_t compDataSize = arch.GetComponentDataSize();

    for (uint16_t i = 0; i < metas[chunkIndex].count; ++i) {
        InvokeWithComponents<AccessModes...>(
            actors[i],
            compBase + i * compDataSize,
            compDataSize,
            offsets,
            std::forward<Func>(func),
            std::index_sequence_for<AccessModes...>{}
        );
    }
}

// オフセット配列を構築
template<typename... AccessModes>
std::array<size_t, sizeof...(AccessModes)> GetOffsets(const Archetype& arch) {
    return { arch.GetComponentOffset<unwrap_access_t<AccessModes>>()... };
}

} // namespace detail

//============================================================================
//! @brief TypedQuery
//!
//! In/Out/InOut対応の型安全クエリ。
//! アクセスモードに基づいてconst/非constを自動決定。
//!
//! @tparam AccessModes アクセスモード群（In<T>, Out<T>, InOut<T>）
//!
//! @code
//! registry.Query<InOut<TransformData>, In<VelocityData>>()
//!     .ForEach([](Actor e, TransformData& t, const VelocityData& v) {
//!         t.position += v.velocity;
//!     });
//! @endcode
//============================================================================
template<typename... AccessModes>
class TypedQuery {
public:
    // 全てアクセスモードであることを検証
    static_assert((is_access_mode_v<AccessModes> && ...),
        "All type parameters must be In<T>, Out<T>, or InOut<T>");

    // 最大7コンポーネント
    static_assert(sizeof...(AccessModes) <= 7,
        "Maximum 7 components supported");

    // 最低1コンポーネント
    static_assert(sizeof...(AccessModes) >= 1,
        "At least one component required");

    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param registry ActorRegistryへのポインタ
    //------------------------------------------------------------------------
    explicit TypedQuery(ActorRegistry* registry) noexcept
        : registry_(registry) {}

    //------------------------------------------------------------------------
    //! @brief 全マッチActorに対してイテレーション
    //! @tparam Func 処理関数の型
    //! @param func void(Actor, T&...) または void(Actor, const T&...)
    //!
    //! In<T> の場合は const T&、Out<T>/InOut<T> の場合は T& として渡される。
    //------------------------------------------------------------------------
    template<typename Func>
    void ForEach(Func&& func);

    //------------------------------------------------------------------------
    //! @brief 必須コンポーネントフィルタを追加
    //! @tparam Ts 必須コンポーネント型群
    //! @return 自身への参照
    //!
    //! @note アクセスモードで指定したコンポーネント以外に、
    //!       追加で必要なコンポーネントを指定する場合に使用
    //------------------------------------------------------------------------
    template<typename... Ts>
    TypedQuery& With() {
        (withTypes_.push_back(std::type_index(typeid(Ts))), ...);
        return *this;
    }

    //------------------------------------------------------------------------
    //! @brief 除外コンポーネントフィルタを追加
    //! @tparam Ts 除外コンポーネント型群
    //! @return 自身への参照
    //!
    //! @note 指定したコンポーネントを持たないActorのみを処理
    //------------------------------------------------------------------------
    template<typename... Ts>
    TypedQuery& Without() {
        (withoutTypes_.push_back(std::type_index(typeid(Ts))), ...);
        return *this;
    }

    //------------------------------------------------------------------------
    //! @brief フィルター条件を取得（内部用）
    //------------------------------------------------------------------------
    [[nodiscard]] const std::vector<std::type_index>& GetWithTypes() const noexcept {
        return withTypes_;
    }

    [[nodiscard]] const std::vector<std::type_index>& GetWithoutTypes() const noexcept {
        return withoutTypes_;
    }

private:
    ActorRegistry* registry_;
    std::vector<std::type_index> withTypes_;     //!< 必須コンポーネント型
    std::vector<std::type_index> withoutTypes_;  //!< 除外コンポーネント型
};

} // namespace ECS

// 実装（ActorRegistryの完全定義が必要）
#include "../detail/typed_query_impl.h"
