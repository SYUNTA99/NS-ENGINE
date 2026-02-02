//----------------------------------------------------------------------------
//! @file   archetype_storage.h
//! @brief  ECS ArchetypeStorage - 全Archetypeの一元管理
//----------------------------------------------------------------------------
#pragma once

#include "common/stl/stl_common.h"
#include "common/stl/stl_containers.h"
#include "common/stl/stl_metaprogramming.h"
#include "common/utility/non_copyable.h"
#include "archetype.h"
#include "query/query_cache.h"
#include "query/query_types.h"

namespace ECS {

namespace detail {

//============================================================================
// Archetype マッチングヘルパー
//============================================================================

//! タプル内の全ての型をArchetypeが持っているか確認
template<typename Tuple>
struct HasAllInTuple;

template<>
struct HasAllInTuple<std::tuple<>> {
    static bool Check(const Archetype&) { return true; }
};

template<typename T, typename... Rest>
struct HasAllInTuple<std::tuple<T, Rest...>> {
    static bool Check(const Archetype& arch) {
        if (!arch.HasComponent<T>()) return false;
        return HasAllInTuple<std::tuple<Rest...>>::Check(arch);
    }
};

//! タプル内のいずれかの型をArchetypeが持っているか確認
template<typename Tuple>
struct HasAnyInTuple;

template<>
struct HasAnyInTuple<std::tuple<>> {
    static bool Check(const Archetype&) { return false; }
};

template<typename T, typename... Rest>
struct HasAnyInTuple<std::tuple<T, Rest...>> {
    static bool Check(const Archetype& arch) {
        if (arch.HasComponent<T>()) return true;
        return HasAnyInTuple<std::tuple<Rest...>>::Check(arch);
    }
};

//! 純粋なRequired型を全て持ち、Exclude型を持たないか確認
template<typename... Ts>
bool ArchetypeMatches(const Archetype& arch) {
    using PureRequired = filter_pure_required_t<Ts...>;
    using Excludes = filter_excludes_t<Ts...>;

    // 純粋なRequired型を全て持つ
    if (!HasAllInTuple<PureRequired>::Check(arch)) {
        return false;
    }

    // Exclude型を1つも持たない
    if (HasAnyInTuple<Excludes>::Check(arch)) {
        return false;
    }

    return true;
}

} // namespace detail

//============================================================================
//! @brief ArchetypeStorage
//!
//! 全Archetypeを管理する中央ストレージ。
//! ArchetypeIdによる検索と、必要に応じた新規Archetype作成を行う。
//!
//! @note メインスレッドからのみ操作すること。
//!       並列処理中の構造変更は禁止（Deferred操作を使用）。
//============================================================================
class ArchetypeStorage : private NonCopyable {
public:
    //! 空のArchetype用の特別なID
    //! FNV-1aハッシュの初期値より大きい値を使用し、通常のハッシュと衝突しないようにする
    static constexpr ArchetypeId kEmptyArchetypeId = UINT64_MAX;

    ArchetypeStorage() = default;
    ~ArchetypeStorage() = default;

    //------------------------------------------------------------------------
    //! @brief 指定のコンポーネント構成に対応するArchetypeを取得または作成
    //! @tparam Ts コンポーネント型群
    //! @return Archetypeへのポインタ（所有権はArchetypeStorageが保持）
    //------------------------------------------------------------------------
    template<typename... Ts>
    Archetype* GetOrCreate() {
        ArchetypeId id = Archetype::CalculateId<Ts...>();

        auto it = archetypes_.find(id);
        if (it != archetypes_.end()) {
            return it->second.get();
        }

        // 新規作成
        auto archetype = BuildArchetype<Ts...>();
        Archetype* ptr = archetype.get();
        archetypes_[id] = std::move(archetype);
        queryCache_.Invalidate();  // キャッシュ無効化

        return ptr;
    }

    //------------------------------------------------------------------------
    //! @brief ArchetypeIdでArchetypeを検索
    //! @param id ArchetypeId
    //! @return Archetypeへのポインタ（存在しない場合はnullptr）
    //------------------------------------------------------------------------
    [[nodiscard]] Archetype* Get(ArchetypeId id) const noexcept {
        auto it = archetypes_.find(id);
        return it != archetypes_.end() ? it->second.get() : nullptr;
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネント情報配列からArchetypeを取得または作成
    //! @param components コンポーネント情報配列（ソート済み）
    //! @return Archetypeへのポインタ
    //------------------------------------------------------------------------
    Archetype* GetOrCreate(std::vector<ComponentInfo> components) {
        // ソート
        std::sort(components.begin(), components.end());
        ArchetypeId id = Archetype::CalculateId(components);

        auto it = archetypes_.find(id);
        if (it != archetypes_.end()) {
            return it->second.get();
        }

        auto archetype = std::make_unique<Archetype>(std::move(components));
        Archetype* ptr = archetype.get();
        archetypes_[id] = std::move(archetype);
        queryCache_.Invalidate();  // キャッシュ無効化

        return ptr;
    }

    //------------------------------------------------------------------------
    //! @brief 空のArchetype（コンポーネントなし）を取得または作成
    //! @return 空のArchetypeへのポインタ
    //------------------------------------------------------------------------
    Archetype* GetOrCreateEmpty() {
        auto it = archetypes_.find(kEmptyArchetypeId);
        if (it != archetypes_.end()) {
            return it->second.get();
        }

        // 空のArchetypeを作成（コンポーネントなし）
        auto archetype = std::make_unique<Archetype>();
        Archetype* ptr = archetype.get();
        archetypes_[kEmptyArchetypeId] = std::move(archetype);
        queryCache_.Invalidate();  // キャッシュ無効化

        return ptr;
    }

    //------------------------------------------------------------------------
    //! @brief 既存Archetypeに型Tを追加した新Archetypeを取得または作成
    //! @tparam T 追加するコンポーネント型
    //! @param base 基となるArchetype（nullptrの場合は空）
    //! @return 新しいArchetypeへのポインタ
    //------------------------------------------------------------------------
    template<typename T>
    Archetype* GetOrCreateWith(Archetype* base) {
        // baseのコンポーネント + T
        std::vector<ComponentInfo> newComponents;
        if (base) {
            newComponents = base->GetComponents();
        }

        // Tが既に存在するか確認
        std::type_index newType(typeid(T));
        for (const auto& info : newComponents) {
            if (info.type == newType) {
                // 既に持っている場合は同じArchetypeを返す
                return base;
            }
        }

        // Tを追加（Tagコンポーネントはサイズ0として扱う）
        constexpr size_t size = is_tag_component_v<T> ? 0 : sizeof(T);
        constexpr size_t align = is_tag_component_v<T> ? 1 : alignof(T);
        newComponents.emplace_back(newType, size, align);

        return GetOrCreate(std::move(newComponents));
    }

    //------------------------------------------------------------------------
    //! @brief 既存Archetypeから型Tを削除した新Archetypeを取得または作成
    //! @tparam T 削除するコンポーネント型
    //! @param base 基となるArchetype
    //! @return 新しいArchetypeへのポインタ（Tがない場合は空Archetype）
    //------------------------------------------------------------------------
    template<typename T>
    Archetype* GetOrCreateWithout(Archetype* base) {
        if (!base) {
            return GetOrCreateEmpty();
        }

        // baseのコンポーネント - T
        std::vector<ComponentInfo> newComponents;
        std::type_index removeType(typeid(T));

        for (const auto& info : base->GetComponents()) {
            if (info.type != removeType) {
                newComponents.push_back(info);
            }
        }

        // 結果が空の場合は空Archetypeを返す
        if (newComponents.empty()) {
            return GetOrCreateEmpty();
        }

        return GetOrCreate(std::move(newComponents));
    }

    //------------------------------------------------------------------------
    //! @brief 既存ArchetypeにDynamicBuffer<T>を追加した新Archetypeを取得または作成
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param base 基となるArchetype（nullptrの場合は空）
    //! @return 新しいArchetypeへのポインタ
    //------------------------------------------------------------------------
    template<typename T>
    Archetype* GetOrCreateWithBuffer(Archetype* base) {
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        // baseのコンポーネント + Buffer<T>
        std::vector<ComponentInfo> newComponents;
        if (base) {
            newComponents = base->GetComponents();
        }

        // バッファが既に存在するか確認
        std::type_index newType(typeid(T));
        for (const auto& info : newComponents) {
            if (info.type == newType && info.isBuffer) {
                // 既に持っている場合は同じArchetypeを返す
                return base;
            }
        }

        // バッファを追加
        constexpr int32_t inlineCap = InternalBufferCapacity<T>::Value;
        constexpr size_t totalSize = sizeof(BufferHeader) +
                                     static_cast<size_t>(inlineCap) * sizeof(T);

        newComponents.emplace_back(
            newType,
            totalSize,
            alignof(BufferHeader),
            sizeof(T),   // elementSize
            inlineCap    // inlineCapacity
        );

        return GetOrCreate(std::move(newComponents));
    }

    //------------------------------------------------------------------------
    //! @brief 既存ArchetypeからDynamicBuffer<T>を削除した新Archetypeを取得または作成
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param base 基となるArchetype
    //! @return 新しいArchetypeへのポインタ（バッファがない場合は同じArchetype）
    //------------------------------------------------------------------------
    template<typename T>
    Archetype* GetOrCreateWithoutBuffer(Archetype* base) {
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        if (!base) {
            return GetOrCreateEmpty();
        }

        // baseのコンポーネント - Buffer<T>
        std::vector<ComponentInfo> newComponents;
        std::type_index removeType(typeid(T));

        for (const auto& info : base->GetComponents()) {
            // バッファコンポーネントで型が一致するものを除外
            if (!(info.type == removeType && info.isBuffer)) {
                newComponents.push_back(info);
            }
        }

        // 結果が空の場合は空Archetypeを返す
        if (newComponents.empty()) {
            return GetOrCreateEmpty();
        }

        return GetOrCreate(std::move(newComponents));
    }

    //------------------------------------------------------------------------
    //! @brief 登録されているArchetype数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetArchetypeCount() const noexcept {
        return archetypes_.size();
    }

    //------------------------------------------------------------------------
    //! @brief 全Actorの総数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetTotalActorCount() const noexcept {
        size_t total = 0;
        for (const auto& [id, archetype] : archetypes_) {
            total += archetype->GetActorCount();
        }
        return total;
    }

    //------------------------------------------------------------------------
    //! @brief 全Archetypeをイテレーション
    //! @tparam Func 処理関数の型 void(Archetype&)
    //! @param func 各Archetypeに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename Func>
    void ForEach(Func&& func) {
        for (auto& [id, archetype] : archetypes_) {
            func(*archetype);
        }
    }

    template<typename Func>
    void ForEach(Func&& func) const {
        for (const auto& [id, archetype] : archetypes_) {
            func(*archetype);
        }
    }

    //------------------------------------------------------------------------
    //! @brief 指定コンポーネントを持つ全Archetypeをイテレーション
    //! @tparam Ts 必須コンポーネント型群
    //! @tparam Func 処理関数の型 void(Archetype&)
    //! @param func 各Archetypeに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename... Ts, typename Func>
    void ForEachMatching(Func&& func) {
        for (auto& [id, archetype] : archetypes_) {
            if ((archetype->HasComponent<Ts>() && ...)) {
                func(*archetype);
            }
        }
    }

    template<typename... Ts, typename Func>
    void ForEachMatching(Func&& func) const {
        for (const auto& [id, archetype] : archetypes_) {
            if ((archetype->HasComponent<Ts>() && ...)) {
                func(*archetype);
            }
        }
    }

    //------------------------------------------------------------------------
    //! @brief Optional/Exclude対応のArchetypeイテレーション
    //!
    //! @tparam Ts コンポーネント型群（T, Optional<T>, Exclude<T> の組み合わせ）
    //! @tparam Func 処理関数の型 void(Archetype&)
    //! @param func 各Archetypeに対して呼び出す関数
    //!
    //! 例:
    //! ```cpp
    //! // TransformDataを持ち、Deadを持たないArchetypeをイテレーション
    //! storage.ForEachMatchingFiltered<TransformData, Exclude<Dead>>(func);
    //! ```
    //------------------------------------------------------------------------
    template<typename... Ts, typename Func>
    void ForEachMatchingFiltered(Func&& func) {
        for (auto& [id, archetype] : archetypes_) {
            if (detail::ArchetypeMatches<Ts...>(*archetype)) {
                func(*archetype);
            }
        }
    }

    template<typename... Ts, typename Func>
    void ForEachMatchingFiltered(Func&& func) const {
        for (const auto& [id, archetype] : archetypes_) {
            if (detail::ArchetypeMatches<Ts...>(*archetype)) {
                func(*archetype);
            }
        }
    }

    //------------------------------------------------------------------------
    //! @brief 指定コンポーネントを持つ全Archetypeをキャッシュ経由でイテレーション
    //!
    //! @tparam Ts コンポーネント型群（T, Optional<T>, Exclude<T> の組み合わせ）
    //! @tparam Func 処理関数の型 void(Archetype&)
    //! @param func 各Archetypeに対して呼び出す関数
    //!
    //! @note キャッシュにヒットした場合O(マッチArchetype数)、
    //!       ミスした場合O(全Archetype数)で再構築
    //------------------------------------------------------------------------
    template<typename... Ts, typename Func>
    void ForEachMatchingCached(Func&& func) {
        constexpr size_t key = QueryCache::CalculateKey<Ts...>();

        // キャッシュを確認
        QueryCache::CacheEntry* entry = queryCache_.GetEntry(key);
        if (entry) {
            // キャッシュヒット
            for (Archetype* arch : entry->archetypes) {
                func(*arch);
            }
            return;
        }

        // キャッシュミス: 再構築（Optional/Exclude対応）
        std::vector<Archetype*> matching;
        for (auto& [id, archetype] : archetypes_) {
            if (detail::ArchetypeMatches<Ts...>(*archetype)) {
                matching.push_back(archetype.get());
            }
        }

        // キャッシュに保存
        queryCache_.SetEntry(key, matching);

        // 処理実行
        for (Archetype* arch : matching) {
            func(*arch);
        }
    }

    //------------------------------------------------------------------------
    //! @brief QueryCacheへの参照を取得
    //------------------------------------------------------------------------
    [[nodiscard]] QueryCache& GetQueryCache() noexcept {
        return queryCache_;
    }

    [[nodiscard]] const QueryCache& GetQueryCache() const noexcept {
        return queryCache_;
    }

    //------------------------------------------------------------------------
    //! @brief 全データをクリア
    //------------------------------------------------------------------------
    void Clear() {
        archetypes_.clear();
        queryCache_.Clear();
    }

    //------------------------------------------------------------------------
    //! @brief 現在の書き込みバージョンを設定（ForEach呼び出し前に設定）
    //! @param version フレームカウンターなどの値
    //------------------------------------------------------------------------
    void SetWriteVersion(uint32_t version) noexcept {
        currentWriteVersion_ = version;
    }

    //------------------------------------------------------------------------
    //! @brief 現在の書き込みバージョンを取得
    //! @return 現在の書き込みバージョン
    //------------------------------------------------------------------------
    [[nodiscard]] uint32_t GetWriteVersion() const noexcept {
        return currentWriteVersion_;
    }

private:
    //------------------------------------------------------------------------
    //! @brief 型リストからArchetypeを構築
    //------------------------------------------------------------------------
    template<typename... Ts>
    static std::unique_ptr<Archetype> BuildArchetype() {
        ArchetypeBuilder builder;
        (builder.Add<Ts>(), ...);
        return builder.Build();
    }

private:
    std::unordered_map<ArchetypeId, std::unique_ptr<Archetype>> archetypes_;
    mutable QueryCache queryCache_;  //!< Queryマッチング結果キャッシュ
    uint32_t currentWriteVersion_ = 0;  //!< ForEach内での書き込みバージョン
};

} // namespace ECS
