//----------------------------------------------------------------------------
//! @file   actor_registry.h
//! @brief  ECS ActorRegistry - Actor/Component管理
//----------------------------------------------------------------------------
#pragma once

#include "common/stl/stl_common.h"
#include "common/stl/stl_metaprogramming.h"
#include "common/utility/non_copyable.h"
#include "access_mode.h"
#include "actor.h"
#include "actor_manager.h"
#include "actor_record.h"
#include "archetype_storage.h"
#include "ecs_assert.h"

namespace ECS {

// 前方宣言
template<typename... AccessModes> class TypedQuery;

//============================================================================
//! @brief ActorRegistry
//!
//! Actor/Componentの管理を担当するクラス。
//! Worldから分離された責務:
//! - Actor生成/破棄
//! - Component追加/取得/削除
//! - クエリ/イテレーション
//!
//! @note 構造変更（Create/Destroy/Add/Remove）はメインスレッドのみ。
//!       並列処理中はDeferred操作を使用すること。
//============================================================================
class ActorRegistry : private NonCopyable {
public:
    ActorRegistry() = default;
    ~ActorRegistry() = default;

    //========================================================================
    // Actor操作
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 新しいアクターを生成
    //! @return 生成されたアクター
    //------------------------------------------------------------------------
    [[nodiscard]] Actor Create() {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();
        Actor actor = entities_.Create();

        // 空のArchetypeにActorを割り当て
        Archetype* emptyArch = archetypes_.GetOrCreateEmpty();
        uint32_t chunkIndex;
        uint16_t indexInChunk;
        emptyArch->AllocateActor(actor, chunkIndex, indexInChunk);

        // ActorRecordを設定
        entities_.SetRecord(actor, emptyArch, chunkIndex, indexInChunk);

        return actor;
    }

    //------------------------------------------------------------------------
    //! @brief 複数アクターを一括生成（コンポーネントなし）
    //! @param count 生成数
    //! @return 生成されたアクター配列
    //------------------------------------------------------------------------
    [[nodiscard]] std::vector<Actor> Create(size_t count) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        std::vector<Actor> actors = entities_.CreateBatch(count);
        Archetype* emptyArch = archetypes_.GetOrCreateEmpty();
        std::vector<std::pair<uint32_t, uint16_t>> positions;
        emptyArch->AllocateActors(actors, positions);

        for (size_t i = 0; i < actors.size(); ++i) {
            entities_.SetRecord(actors[i], emptyArch, positions[i].first, positions[i].second);
        }

        return actors;
    }

    //------------------------------------------------------------------------
    //! @brief 複数アクターを一括生成（コンポーネント付き）
    //! @tparam Ts 初期コンポーネント型群
    //! @param count 生成数
    //! @return 生成されたアクター配列
    //------------------------------------------------------------------------
    template<typename... Ts>
    [[nodiscard]] std::vector<Actor> Create(size_t count) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        static_assert((std::is_trivially_copyable_v<Ts> && ...),
            "ECS components must be trivially copyable");

        std::vector<Actor> actors = entities_.CreateBatch(count);
        Archetype* arch = archetypes_.GetOrCreate<Ts...>();
        std::vector<std::pair<uint32_t, uint16_t>> positions;
        arch->AllocateActors(actors, positions);

        for (size_t i = 0; i < actors.size(); ++i) {
            entities_.SetRecord(actors[i], arch, positions[i].first, positions[i].second);
            ((new (arch->GetComponent<Ts>(positions[i].first, positions[i].second)) Ts()), ...);
        }

        return actors;
    }

    //------------------------------------------------------------------------
    //! @brief アクターを破棄
    //! @param actor 破棄するアクター
    //------------------------------------------------------------------------
    void Destroy(Actor actor) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();
        if (!entities_.IsAlive(actor)) {
            return;
        }

        ActorRecord& rec = entities_.GetRecord(actor);
        if (rec.archetype) {
            uint16_t movedFromIndex = rec.archetype->DeallocateActor(
                rec.chunkIndex, rec.indexInChunk);

            if (movedFromIndex != UINT16_MAX) {
                Actor movedActor = rec.archetype->GetActorAt(rec.chunkIndex, rec.indexInChunk);
                if (movedActor.IsValid()) {
                    ActorRecord& movedRec = entities_.GetRecord(movedActor);
                    movedRec.indexInChunk = rec.indexInChunk;
                }
            }

            rec.Clear();
        }

        entities_.Destroy(actor);
    }

    //------------------------------------------------------------------------
    //! @brief アクターが生存しているか確認
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsAlive(Actor actor) const {
        return entities_.IsAlive(actor);
    }

    //------------------------------------------------------------------------
    //! @brief 生存アクター数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Count() const noexcept {
        return entities_.Count();
    }

    //========================================================================
    // Component操作
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief コンポーネントを追加
    //! @tparam T コンポーネントの型
    //! @tparam Args コンストラクタ引数の型
    //! @param actor 追加先のアクター
    //! @param args コンストラクタ引数
    //! @return 追加されたコンポーネントへのポインタ
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    T* Add(Actor actor, Args&&... args) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        static_assert(std::is_trivially_copyable_v<T>,
            "ECS components must be trivially copyable for memcpy-based Archetype migration");

        if (!entities_.IsAlive(actor)) {
            return nullptr;
        }

        ActorRecord& rec = entities_.GetRecord(actor);
        Archetype* oldArch = rec.archetype;
        Archetype* newArch = archetypes_.GetOrCreateWith<T>(oldArch);

        if (newArch == oldArch) {
            return newArch->GetComponent<T>(rec.chunkIndex, rec.indexInChunk);
        }

        Actor swappedActor;
        auto [newChunkIndex, newIndexInChunk] = newArch->MoveActorFrom(
            oldArch, rec.chunkIndex, rec.indexInChunk, actor, swappedActor);

        if (swappedActor.IsValid()) {
            ActorRecord& swappedRec = entities_.GetRecord(swappedActor);
            swappedRec.indexInChunk = rec.indexInChunk;
        }

        rec.archetype = newArch;
        rec.chunkIndex = newChunkIndex;
        rec.indexInChunk = newIndexInChunk;

        T* comp = newArch->GetComponent<T>(newChunkIndex, newIndexInChunk);
        try {
            new (comp) T(std::forward<Args>(args)...);
        } catch (...) {
            Actor rollbackSwapped;
            auto [rbChunkIndex, rbIndexInChunk] = oldArch->MoveActorFrom(
                newArch, newChunkIndex, newIndexInChunk, actor, rollbackSwapped);

            if (rollbackSwapped.IsValid()) {
                ActorRecord& rbSwappedRec = entities_.GetRecord(rollbackSwapped);
                rbSwappedRec.indexInChunk = rbIndexInChunk;
            }

            rec.archetype = oldArch;
            rec.chunkIndex = rbChunkIndex;
            rec.indexInChunk = rbIndexInChunk;

            throw;
        }

        return comp;
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを取得
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //! @return コンポーネントへのポインタ（存在しない場合はnullptr）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* Get(Actor actor) {
        if (!entities_.IsAlive(actor)) {
            return nullptr;
        }

        const ActorRecord& rec = entities_.GetRecord(actor);
        if (!rec.archetype) {
            return nullptr;
        }

        return rec.archetype->GetComponent<T>(rec.chunkIndex, rec.indexInChunk);
    }

    template<typename T>
    [[nodiscard]] const T* Get(Actor actor) const {
        if (!entities_.IsAlive(actor)) {
            return nullptr;
        }

        const ActorRecord& rec = entities_.GetRecord(actor);
        if (!rec.archetype) {
            return nullptr;
        }

        return rec.archetype->GetComponent<T>(rec.chunkIndex, rec.indexInChunk);
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを所持しているか確認
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool Has(Actor actor) const {
        if (!entities_.IsAlive(actor)) {
            return false;
        }

        const ActorRecord& rec = entities_.GetRecord(actor);
        if (!rec.archetype) {
            return false;
        }

        return rec.archetype->HasComponent<T>();
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを削除
    //------------------------------------------------------------------------
    template<typename T>
    void Remove(Actor actor) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        if (!entities_.IsAlive(actor)) {
            return;
        }

        ActorRecord& rec = entities_.GetRecord(actor);
        Archetype* oldArch = rec.archetype;
        if (!oldArch || !oldArch->HasComponent<T>()) {
            return;
        }

        Archetype* newArch = archetypes_.GetOrCreateWithout<T>(oldArch);

        Actor swappedActor;
        auto [newChunkIndex, newIndexInChunk] = newArch->MoveActorFrom(
            oldArch, rec.chunkIndex, rec.indexInChunk, actor, swappedActor);

        if (swappedActor.IsValid()) {
            ActorRecord& swappedRec = entities_.GetRecord(swappedActor);
            swappedRec.indexInChunk = rec.indexInChunk;
        }

        rec.archetype = newArch;
        rec.chunkIndex = newChunkIndex;
        rec.indexInChunk = newIndexInChunk;
    }

    //========================================================================
    // DynamicBuffer操作
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを取得
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param actor 対象のアクター
    //! @return DynamicBuffer<T>（存在しない場合は無効なバッファ）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] DynamicBuffer<T> GetBuffer(Actor actor) {
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        if (!entities_.IsAlive(actor)) {
            return DynamicBuffer<T>();
        }

        const ActorRecord& rec = entities_.GetRecord(actor);
        if (!rec.archetype) {
            return DynamicBuffer<T>();
        }

        return rec.archetype->GetBuffer<T>(rec.chunkIndex, rec.indexInChunk);
    }

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを取得（const版）
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param actor 対象のアクター
    //! @return ConstDynamicBuffer<T>（存在しない場合は無効なバッファ）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] ConstDynamicBuffer<T> GetBuffer(Actor actor) const {
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        if (!entities_.IsAlive(actor)) {
            return ConstDynamicBuffer<T>();
        }

        const ActorRecord& rec = entities_.GetRecord(actor);
        if (!rec.archetype) {
            return ConstDynamicBuffer<T>();
        }

        return rec.archetype->GetConstBuffer<T>(rec.chunkIndex, rec.indexInChunk);
    }

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを所持しているか確認
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param actor 対象のアクター
    //! @return バッファを所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool HasBuffer(Actor actor) const {
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        if (!entities_.IsAlive(actor)) {
            return false;
        }

        const ActorRecord& rec = entities_.GetRecord(actor);
        if (!rec.archetype) {
            return false;
        }

        return rec.archetype->HasBuffer<T>();
    }

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを追加
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param actor 追加先のアクター
    //! @return DynamicBuffer<T>アクセサ（追加失敗時は無効なバッファ）
    //!
    //! @note 既にバッファを所持している場合は既存のバッファを返す
    //------------------------------------------------------------------------
    template<typename T>
    DynamicBuffer<T> AddBuffer(Actor actor) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        if (!entities_.IsAlive(actor)) {
            return DynamicBuffer<T>();
        }

        ActorRecord& rec = entities_.GetRecord(actor);
        Archetype* oldArch = rec.archetype;

        // バッファ付きのArchetypeを取得
        Archetype* newArch = archetypes_.GetOrCreateWithBuffer<T>(oldArch);

        // 同じArchetypeの場合は既存のバッファを返す
        if (newArch == oldArch) {
            return oldArch->GetBuffer<T>(rec.chunkIndex, rec.indexInChunk);
        }

        // 新しいArchetypeに移動
        Actor swappedActor;
        auto [newChunkIndex, newIndexInChunk] = newArch->MoveActorFrom(
            oldArch, rec.chunkIndex, rec.indexInChunk, actor, swappedActor);

        if (swappedActor.IsValid()) {
            ActorRecord& swappedRec = entities_.GetRecord(swappedActor);
            swappedRec.indexInChunk = rec.indexInChunk;
        }

        rec.archetype = newArch;
        rec.chunkIndex = newChunkIndex;
        rec.indexInChunk = newIndexInChunk;

        // 新しいバッファを返す
        return newArch->GetBuffer<T>(newChunkIndex, newIndexInChunk);
    }

    //========================================================================
    // クエリ/イテレーション
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 型安全なクエリを構築（In/Out/InOut対応）
    //! @tparam AccessModes アクセスモード群（In<T>, Out<T>, InOut<T>）
    //! @return TypedQueryオブジェクト
    //!
    //! @code
    //! world.Actors().Query<InOut<TransformData>, In<VelocityData>>()
    //!     .ForEach([](Actor e, TransformData& t, const VelocityData& v) {
    //!         t.position += v.velocity;
    //!     });
    //! @endcode
    //------------------------------------------------------------------------
    template<typename... AccessModes>
    [[nodiscard]] TypedQuery<AccessModes...> Query();

    //------------------------------------------------------------------------
    //! @brief ActorRecordへのアクセス（内部用）
    //------------------------------------------------------------------------
    [[nodiscard]] ActorRecord& GetRecord(Actor actor) {
        return entities_.GetRecord(actor);
    }

    [[nodiscard]] const ActorRecord& GetRecord(Actor actor) const {
        return entities_.GetRecord(actor);
    }

    //------------------------------------------------------------------------
    //! @brief ArchetypeStorageへのアクセス（内部用）
    //------------------------------------------------------------------------
    [[nodiscard]] ArchetypeStorage& GetArchetypeStorage() noexcept {
        return archetypes_;
    }

    [[nodiscard]] const ArchetypeStorage& GetArchetypeStorage() const noexcept {
        return archetypes_;
    }

    //------------------------------------------------------------------------
    //! @brief 全データをクリア
    //------------------------------------------------------------------------
    void Clear() {
        entities_.Clear();
        archetypes_.Clear();
    }

    //========================================================================
    // 低レベルAPI（Prefab/Instantiate用）
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief Actor IDのみを生成（Archetype割り当てなし）
    //! @return 生成されたアクター
    //!
    //! 注意: SetRecord()で必ずArchetypeを設定すること
    //------------------------------------------------------------------------
    [[nodiscard]] Actor CreateActorId() {
        return entities_.Create();
    }

    //------------------------------------------------------------------------
    //! @brief Actor IDを一括生成（Archetype割り当てなし）
    //! @param count 生成数
    //! @return 生成されたアクター配列
    //!
    //! 注意: SetRecord()で必ずArchetypeを設定すること
    //------------------------------------------------------------------------
    [[nodiscard]] std::vector<Actor> CreateActorIds(size_t count) {
        return entities_.CreateBatch(count);
    }

    //------------------------------------------------------------------------
    //! @brief ActorRecordを設定（低レベルAPI）
    //! @param actor 対象のアクター
    //! @param arch Archetype
    //! @param chunkIndex チャンクインデックス
    //! @param indexInChunk チャンク内インデックス
    //------------------------------------------------------------------------
    void SetRecord(Actor actor, Archetype* arch, uint32_t chunkIndex, uint16_t indexInChunk) {
        entities_.SetRecord(actor, arch, chunkIndex, indexInChunk);
    }

private:
    ActorManager entities_;       //!< Actor ID/世代管理
    ArchetypeStorage archetypes_; //!< Archetype管理
};

} // namespace ECS

// TypedQueryの実装（ActorRegistryの完全定義が必要）
#include "query/typed_query.h"
