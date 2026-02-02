//----------------------------------------------------------------------------
//! @file   world.h
//! @brief  ECS World - アクター・コンポーネント・システムの中央管理
//----------------------------------------------------------------------------
#pragma once

#include "common/stl/stl_common.h"
#include "common/stl/stl_containers.h"
#include "common/stl/stl_metaprogramming.h"
#include "common/utility/non_copyable.h"
#include "actor.h"
#include "actor_manager.h"
#include "actor_record.h"
#include "actor_registry.h"
#include "archetype_storage.h"
#include "prefab.h"
#include "query/cached_query.h"
#include "component_ref.h"
#include "deferred_context.h"
#include "deferred_queue.h"
#include "detail/foreach_helpers.h"
#include "ecs_assert.h"
#include "query/query.h"
#include "system.h"
#include "system_graph.h"
#include "system_scheduler.h"
#include "world_container.h"
#include "typed_foreach.h"
#include "buffer/dynamic_buffer.h"
#include "components/transform/transform_components.h"

// 前方宣言
class GameObject;
class JobHandle;

namespace ECS {

//============================================================================
//! @brief ECSワールド
//!
//! 世界の管理を担当するクラス。
//!
//! 新しいAPI（推奨）:
//! @code
//! // Actor/Component操作
//! auto actor = world.Actors().Create();
//! world.Actors().Add<TransformData>(actor, position);
//!
//! // クエリ（In/Out/InOut明示）
//! world.Actors().Query<InOut<TransformData>, In<VelocityData>>()
//!     .ForEach([](Actor e, TransformData& t, const VelocityData& v) {
//!         t.position += v.velocity;
//!     });
//!
//! // System登録
//! world.Systems().Register<MovementSystem>(world);
//! @endcode
//!
//! 従来のAPI（後方互換、非推奨）:
//! @code
//! auto actor = world.CreateActor();
//! world.AddComponent<TransformData>(actor, position);
//! world.ForEach<TransformData>([](Actor e, TransformData& t) { ... });
//! @endcode
//============================================================================
class World : private NonCopyable {
public:

    World();   // unique_ptr<GameObject>のためcppで定義
    ~World();  // unique_ptr<GameObject>のためcppで定義

    //========================================================================
    // 新しいAPI（推奨）
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief ActorRegistryへのアクセス
    //! @return ActorRegistryへの参照
    //!
    //! Actor/Component操作、クエリはこちらを使用。
    //! @code
    //! auto actor = world.Actors().Create();
    //! world.Actors().Add<TransformData>(actor, pos);
    //! world.Actors().Query<InOut<TransformData>, In<VelocityData>>()
    //!     .ForEach([](Actor e, TransformData& t, const VelocityData& v) { ... });
    //! @endcode
    //------------------------------------------------------------------------
    [[nodiscard]] ActorRegistry& Actors() noexcept {
        return container_.ECS().GetActorRegistry();
    }

    [[nodiscard]] const ActorRegistry& Actors() const noexcept {
        return container_.ECS().GetActorRegistry();
    }

    //------------------------------------------------------------------------
    //! @brief SystemSchedulerへのアクセス
    //! @return SystemSchedulerへの参照
    //!
    //! System登録はこちらを使用。
    //! @code
    //! world.Systems().Register<MovementSystem>(world);
    //! world.Systems().RegisterRender<SpriteRenderSystem>(world);
    //! @endcode
    //------------------------------------------------------------------------
    [[nodiscard]] SystemScheduler& Systems() noexcept {
        return container_.Systems();
    }

    [[nodiscard]] const SystemScheduler& Systems() const noexcept {
        return container_.Systems();
    }

    //------------------------------------------------------------------------
    //! @brief WorldContainerへのアクセス
    //! @return WorldContainerへの参照
    //!
    //! 統合コンテナへの直接アクセス。ECS/GameObject/System管理を
    //! 一元的に行う場合に使用。
    //! @code
    //! auto& container = world.Container();
    //! auto actor = container.ECS().Create();
    //! container.ECS().Add<TransformData>(actor, pos);
    //! auto* go = container.GameObjects().Create("Player");
    //! @endcode
    //------------------------------------------------------------------------
    [[nodiscard]] WorldContainer& Container() noexcept {
        return container_;
    }

    [[nodiscard]] const WorldContainer& Container() const noexcept {
        return container_;
    }

    //========================================================================
    // 従来のAPI（後方互換、非推奨）
    //========================================================================

    //========================================================================
    // Actor管理
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 新しいアクターを生成
    //! @return 生成されたアクター
    //------------------------------------------------------------------------
    [[nodiscard]] Actor CreateActor() {
        return container_.ECS().Create();
    }

    //------------------------------------------------------------------------
    //! @brief 複数アクターを一括生成（コンポーネントなし）
    //! @param count 生成数
    //! @return 生成されたアクター配列
    //------------------------------------------------------------------------
    [[nodiscard]] std::vector<Actor> CreateActors(size_t count) {
        return container_.ECS().Create(count);
    }

    //------------------------------------------------------------------------
    //! @brief 複数アクターを一括生成（コンポーネント付き）
    //! @tparam Ts 初期コンポーネント型群
    //! @param count 生成数
    //! @return 生成されたアクター配列
    //!
    //! 使用例:
    //! ```cpp
    //! auto actors = world.CreateActors<TransformData, SpriteData>(10000);
    //! ```
    //------------------------------------------------------------------------
    template<typename... Ts>
    [[nodiscard]] std::vector<Actor> CreateActors(size_t count) {
        return container_.ECS().Create<Ts...>(count);
    }

    //========================================================================
    // Prefab
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief PrefabBuilderを作成
    //! @return PrefabBuilder
    //!
    //! @code
    //! auto prefab = world.CreatePrefab()
    //!     .Add<TransformData>(Vector3::Zero)
    //!     .Add<VelocityData>(Vector3::Zero)
    //!     .Build();
    //! @endcode
    //------------------------------------------------------------------------
    [[nodiscard]] PrefabBuilder CreatePrefab() {
        return PrefabBuilder(container_.ECS().GetArchetypeStorage());
    }

    //------------------------------------------------------------------------
    //! @brief Prefabから単一Actorを高速生成（SoA対応）
    //! @param prefab 確定済みPrefab
    //! @return 生成されたActor
    //------------------------------------------------------------------------
    [[nodiscard]] Actor Instantiate(const Prefab& prefab) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        if (!prefab.IsValid()) {
            return Actor::Invalid();
        }

        auto& registry = container_.ECS().GetActorRegistry();

        // 1. Actor IDを生成
        Actor actor = registry.CreateActorId();

        // 2. 対象Archetypeにスロットを確保
        Archetype* arch = prefab.GetArchetype();
        uint32_t chunkIndex;
        uint16_t indexInChunk;
        arch->AllocateActor(actor, chunkIndex, indexInChunk);

        // 3. ActorRecordを設定
        registry.SetRecord(actor, arch, chunkIndex, indexInChunk);

        // 4. SoA: 各コンポーネントを個別にコピー
        prefab.CopyComponentsTo(arch, chunkIndex, indexInChunk);

        return actor;
    }

    //------------------------------------------------------------------------
    //! @brief Prefabから複数Actorを高速一括生成（SoA対応）
    //! @param prefab 確定済みPrefab
    //! @param count 生成数
    //! @return 生成されたActor配列
    //------------------------------------------------------------------------
    [[nodiscard]] std::vector<Actor> Instantiate(const Prefab& prefab, size_t count) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        if (!prefab.IsValid() || count == 0) {
            return {};
        }

        auto& registry = container_.ECS().GetActorRegistry();

        // 1. Actor IDを一括生成
        std::vector<Actor> actors = registry.CreateActorIds(count);

        // 2. Archetypeにスロットを一括確保
        Archetype* arch = prefab.GetArchetype();
        std::vector<std::pair<uint32_t, uint16_t>> positions;
        arch->AllocateActors(actors, positions);

        // 3. ActorRecordを設定し、SoA: 各コンポーネントを個別にコピー
        for (size_t i = 0; i < actors.size(); ++i) {
            registry.SetRecord(actors[i], arch, positions[i].first, positions[i].second);
            prefab.CopyComponentsTo(arch, positions[i].first, positions[i].second);
        }

        return actors;
    }

    //------------------------------------------------------------------------
    //! @brief アクターを破棄（即時実行）
    //! @param actor 破棄するアクター
    //!
    //! 注意: 既存のポインタが無効化される可能性あり。
    //------------------------------------------------------------------------
    void DestroyActor(Actor actor) {
        container_.ECS().Destroy(actor);
    }

    //------------------------------------------------------------------------
    //! @brief アクターが生存しているか確認
    //! @param actor 確認するアクター
    //! @return 生存している場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsAlive(Actor actor) const {
        return container_.ECS().IsAlive(actor);
    }

    //------------------------------------------------------------------------
    //! @brief 生存アクター数を取得
    //! @return 生存アクター数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t ActorCount() const noexcept {
        return container_.ECS().Count();
    }

    //------------------------------------------------------------------------
    //! @brief アクターのレコードを取得（Archetype/Chunk位置情報）
    //! @param actor 対象のアクター
    //! @return ActorRecordへの参照
    //------------------------------------------------------------------------
    [[nodiscard]] ActorRecord& GetActorRecord(Actor actor) {
        return container_.ECS().GetActorRegistry().GetRecord(actor);
    }

    [[nodiscard]] const ActorRecord& GetActorRecord(Actor actor) const {
        return container_.ECS().GetActorRegistry().GetRecord(actor);
    }

    //------------------------------------------------------------------------
    //! @brief ArchetypeStorageへのアクセス
    //! @return ArchetypeStorageへの参照
    //------------------------------------------------------------------------
    [[nodiscard]] ArchetypeStorage& GetArchetypeStorage() noexcept {
        return container_.ECS().GetArchetypeStorage();
    }

    [[nodiscard]] const ArchetypeStorage& GetArchetypeStorage() const noexcept {
        return container_.ECS().GetArchetypeStorage();
    }

    //========================================================================
    // Component管理
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
    T* AddComponent(Actor actor, Args&&... args) {
        return container_.ECS().Add<T>(actor, std::forward<Args>(args)...);
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを取得 (O(1) Archetypeベース)
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //! @return コンポーネントへのポインタ（存在しない場合はnullptr）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponent(Actor actor) {
        return container_.ECS().Get<T>(actor);
    }

    template<typename T>
    [[nodiscard]] const T* GetComponent(Actor actor) const {
        return container_.ECS().Get<T>(actor);
    }

    //------------------------------------------------------------------------
    //! @brief 安全なコンポーネント参照を取得
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //! @return ComponentRef<T> 安全なコンポーネント参照
    //!
    //! フレーム境界をまたいでも安全にコンポーネントにアクセスできる参照を返す。
    //! キャッシュが古くなった場合は自動的に再取得される。
    //!
    //! @code
    //! auto transformRef = world.GetRef<TransformData>(actor);
    //! // フレーム内は高速アクセス
    //! transformRef->position += velocity;
    //!
    //! world.BeginFrame();
    //! // 次フレームでも自動再取得
    //! transformRef->position += velocity;  // 安全
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] ComponentRef<T> GetRef(Actor actor) {
        T* cached = GetComponent<T>(actor);
        return ComponentRef<T>(this, actor, cached, container_.GetFrameCount());
    }

    template<typename T>
    [[nodiscard]] ComponentConstRef<T> GetRef(Actor actor) const {
        const T* cached = GetComponent<T>(actor);
        return ComponentConstRef<T>(this, actor, cached, container_.GetFrameCount());
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを所持しているか確認 (O(1) Archetypeベース)
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //! @return 所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool HasComponent(Actor actor) const {
        return container_.ECS().Has<T>(actor);
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントの有効状態を取得
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //! @return 有効ならtrue（コンポーネントがない場合もfalse）
    //!
    //! @note Enableable Componentは削除せずに無効化できる。
    //!       無効化されたコンポーネントはQueryのデフォルトでは除外される。
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool IsEnabled(Actor actor) const {
        if (!container_.ECS().IsAlive(actor)) {
            return false;
        }

        const ActorRecord& rec = container_.ECS().GetActorRegistry().GetRecord(actor);
        if (!rec.archetype || !rec.archetype->HasComponent<T>()) {
            return false;
        }

        return rec.archetype->IsComponentEnabled<T>(rec.chunkIndex, rec.indexInChunk);
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントの有効状態を設定
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //! @param enabled 有効にするか
    //!
    //! @note Archetype移動は発生しない（高速）。
    //------------------------------------------------------------------------
    template<typename T>
    void SetEnabled(Actor actor, bool enabled) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        if (!container_.ECS().IsAlive(actor)) {
            return;
        }

        ActorRecord& rec = container_.ECS().GetActorRegistry().GetRecord(actor);
        if (!rec.archetype || !rec.archetype->HasComponent<T>()) {
            return;
        }

        rec.archetype->SetComponentEnabled<T>(rec.chunkIndex, rec.indexInChunk, enabled);
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントを削除
    //! @tparam T コンポーネントの型
    //! @param actor 対象のアクター
    //------------------------------------------------------------------------
    template<typename T>
    void RemoveComponent(Actor actor) {
        container_.ECS().Remove<T>(actor);
    }

    //========================================================================
    // DynamicBuffer管理
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを取得
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param actor 対象のアクター
    //! @return DynamicBuffer<T>アクセサ（バッファがない場合は無効なアクセサ）
    //!
    //! @code
    //! struct Waypoint : IBufferElement { float x, y, z; };
    //! ECS_BUFFER_ELEMENT(Waypoint);
    //!
    //! auto buffer = world.GetBuffer<Waypoint>(actor);
    //! if (buffer) {
    //!     for (const auto& wp : buffer) {
    //!         // ...
    //!     }
    //! }
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] DynamicBuffer<T> GetBuffer(Actor actor) {
        return container_.ECS().GetActorRegistry().GetBuffer<T>(actor);
    }

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを所持しているか確認
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param actor 対象のアクター
    //! @return バッファを所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool HasBuffer(Actor actor) const {
        return container_.ECS().GetActorRegistry().HasBuffer<T>(actor);
    }

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを追加
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param actor 追加先のアクター
    //! @return DynamicBuffer<T>アクセサ
    //!
    //! 既にバッファを持っている場合は既存のバッファを返す。
    //! 持っていない場合はArchetype移動を行い、新しいバッファを返す。
    //!
    //! @code
    //! auto buffer = world.AddBuffer<Waypoint>(actor);
    //! buffer.Add({1.0f, 2.0f, 3.0f});
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    DynamicBuffer<T> AddBuffer(Actor actor) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        auto& registry = container_.ECS().GetActorRegistry();
        if (!registry.IsAlive(actor)) {
            return DynamicBuffer<T>();
        }

        ActorRecord& rec = registry.GetRecord(actor);
        Archetype* oldArch = rec.archetype;

        // 新しいArchetypeを取得（Bufferを追加）
        Archetype* newArch = container_.ECS().GetArchetypeStorage().GetOrCreateWithBuffer<T>(oldArch);

        // 既にバッファを持っている場合
        if (newArch == oldArch) {
            return newArch->GetBuffer<T>(rec.chunkIndex, rec.indexInChunk);
        }

        // 新しいArchetypeへ移動
        Actor swappedActor;
        auto [newChunkIndex, newIndexInChunk] = newArch->MoveActorFrom(
            oldArch, rec.chunkIndex, rec.indexInChunk, actor, swappedActor);

        // swap-and-popで別のActorが移動された場合、そのレコードを更新
        if (swappedActor.IsValid()) {
            ActorRecord& swappedRec = registry.GetRecord(swappedActor);
            swappedRec.indexInChunk = rec.indexInChunk;
        }

        // ActorRecordを更新
        rec.archetype = newArch;
        rec.chunkIndex = newChunkIndex;
        rec.indexInChunk = newIndexInChunk;

        // 新しいバッファを取得し、ヘッダーを初期化
        auto buffer = newArch->GetBuffer<T>(newChunkIndex, newIndexInChunk);

        // BufferHeaderを取得して初期化（SoA対応）
        // GetBufferで返されるバッファはヘッダーポインタを持っている
        // 初期化: length=0, inlineCapacity=trait値, externalPtr=nullptr
        size_t compIdx = newArch->GetComponentIndex<T>();
        if (compIdx != SIZE_MAX) {
            const ComponentInfo* bufInfo = newArch->GetComponentInfo<T>();
            if (bufInfo && bufInfo->isBuffer) {
                std::byte* base = static_cast<std::byte*>(
                    newArch->GetComponentAt(newChunkIndex, newIndexInChunk, compIdx));
                BufferHeader* header = reinterpret_cast<BufferHeader*>(base);

                // ヘッダーを初期化
                header->length = 0;
                header->inlineCapacity = bufInfo->inlineCapacity;
                header->externalPtr = nullptr;
                header->externalCapacity = 0;
                header->reserved = 0;
            }
        }

        return buffer;
    }

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを削除
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param actor 対象のアクター
    //!
    //! バッファの外部ストレージがある場合は自動的に解放される。
    //------------------------------------------------------------------------
    template<typename T>
    void RemoveBuffer(Actor actor) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        auto& registry = container_.ECS().GetActorRegistry();
        if (!registry.IsAlive(actor)) {
            return;
        }

        ActorRecord& rec = registry.GetRecord(actor);
        Archetype* oldArch = rec.archetype;
        if (!oldArch) {
            return;
        }

        // バッファを持っていなければ何もしない
        if (!oldArch->HasBuffer<T>()) {
            return;
        }

        // バッファを除いた新しいArchetypeを取得
        Archetype* newArch = container_.ECS().GetArchetypeStorage().GetOrCreateWithoutBuffer<T>(oldArch);

        // 新しいArchetypeへ移動（外部ストレージは移動時にクリーンアップ）
        Actor swappedActor;
        auto [newChunkIndex, newIndexInChunk] = newArch->MoveActorFrom(
            oldArch, rec.chunkIndex, rec.indexInChunk, actor, swappedActor);

        // swap-and-popで別のActorが移動された場合、そのレコードを更新
        if (swappedActor.IsValid()) {
            ActorRecord& swappedRec = registry.GetRecord(swappedActor);
            swappedRec.indexInChunk = rec.indexInChunk;
        }

        // ActorRecordを更新
        rec.archetype = newArch;
        rec.chunkIndex = newChunkIndex;
        rec.indexInChunk = newIndexInChunk;
    }

    //------------------------------------------------------------------------
    //! @brief 遅延操作コンテキストを取得（Fluent API）
    //! @return DeferredContextオブジェクト
    //!
    //! チェーン可能な遅延操作APIを提供。
    //!
    //! @code
    //! world.Deferred()
    //!     .AddComponent<PositionData>(actor, 1.0f, 2.0f, 3.0f)
    //!     .AddComponent<VelocityData>(actor, 10.0f, 0.0f, 0.0f)
    //!     .DestroyActor(enemy);
    //! @endcode
    //------------------------------------------------------------------------
    [[nodiscard]] DeferredContext Deferred() noexcept {
        return DeferredContext(*this, container_.Deferred());
    }

    //========================================================================
    // バッチ操作
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 複数Actorにコンポーネントを一括追加（同一値）
    //! @tparam T コンポーネント型
    //! @tparam Args コンストラクタ引数の型
    //! @param actors 追加先のActor配列
    //! @param args 全Actorに適用するコンストラクタ引数
    //! @return 成功したActor数
    //!
    //! @note Archetype遷移を最適化。同じ元Archetypeのものをグループ化し
    //!       一括処理する。
    //!
    //! @code
    //! auto actors = world.CreateActors(100);
    //! world.AddComponentsBatch<VelocityData>(actors, 0.0f, 0.0f, 0.0f);
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    size_t AddComponentsBatch(const std::vector<Actor>& actors, Args&&... args) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        static_assert(std::is_trivially_copyable_v<T>,
            "ECS components must be trivially copyable");

        if (actors.empty()) return 0;

        auto& registry = container_.ECS().GetActorRegistry();
        auto& archetypes = container_.ECS().GetArchetypeStorage();
        size_t successCount = 0;

        // 元Archetypeでグループ化
        std::unordered_map<Archetype*, std::vector<Actor>> byArchetype;
        for (Actor actor : actors) {
            if (!registry.IsAlive(actor)) continue;
            ActorRecord& rec = registry.GetRecord(actor);
            byArchetype[rec.archetype].push_back(actor);
        }

        // 各グループを一括処理
        for (auto& [srcArch, group] : byArchetype) {
            // 目的Archetypeを取得
            Archetype* dstArch = archetypes.GetOrCreateWith<T>(srcArch);

            if (dstArch == srcArch) {
                // 既にコンポーネントを持っている場合はスキップ
                continue;
            }

            // グループ全体を移動
            for (Actor actor : group) {
                ActorRecord& rec = registry.GetRecord(actor);

                Actor swappedActor;
                auto [newChunkIndex, newIndexInChunk] = dstArch->MoveActorFrom(
                    srcArch, rec.chunkIndex, rec.indexInChunk, actor, swappedActor);

                if (swappedActor.IsValid()) {
                    ActorRecord& swappedRec = registry.GetRecord(swappedActor);
                    swappedRec.indexInChunk = rec.indexInChunk;
                }

                rec.archetype = dstArch;
                rec.chunkIndex = newChunkIndex;
                rec.indexInChunk = newIndexInChunk;

                // コンポーネント初期化
                T* comp = dstArch->GetComponent<T>(newChunkIndex, newIndexInChunk);
                new (comp) T(args...);

                ++successCount;
            }
        }

        return successCount;
    }

    //------------------------------------------------------------------------
    //! @brief 複数Actorにコンポーネントを一括追加（個別初期化）
    //! @tparam T コンポーネント型
    //! @param actors 追加先のActor配列
    //! @param initializer 各Actorのコンポーネントを初期化する関数
    //! @return 成功したActor数
    //!
    //! @code
    //! world.AddComponentsBatch<PositionData>(actors,
    //!     [](size_t idx, Actor) {
    //!         return PositionData(idx * 10.0f, 0.0f, 0.0f);
    //!     });
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    size_t AddComponentsBatch(
        const std::vector<Actor>& actors,
        std::function<T(size_t, Actor)> initializer)
    {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        static_assert(std::is_trivially_copyable_v<T>,
            "ECS components must be trivially copyable");

        if (actors.empty()) return 0;

        auto& registry = container_.ECS().GetActorRegistry();
        auto& archetypes = container_.ECS().GetArchetypeStorage();
        size_t successCount = 0;

        // 元Archetypeでグループ化（インデックス付き）
        std::unordered_map<Archetype*, std::vector<std::pair<size_t, Actor>>> byArchetype;
        for (size_t i = 0; i < actors.size(); ++i) {
            Actor actor = actors[i];
            if (!registry.IsAlive(actor)) continue;
            ActorRecord& rec = registry.GetRecord(actor);
            byArchetype[rec.archetype].emplace_back(i, actor);
        }

        // 各グループを一括処理
        for (auto& [srcArch, group] : byArchetype) {
            Archetype* dstArch = archetypes.GetOrCreateWith<T>(srcArch);

            if (dstArch == srcArch) {
                continue;
            }

            for (auto& [idx, actor] : group) {
                ActorRecord& rec = registry.GetRecord(actor);

                Actor swappedActor;
                auto [newChunkIndex, newIndexInChunk] = dstArch->MoveActorFrom(
                    srcArch, rec.chunkIndex, rec.indexInChunk, actor, swappedActor);

                if (swappedActor.IsValid()) {
                    ActorRecord& swappedRec = registry.GetRecord(swappedActor);
                    swappedRec.indexInChunk = rec.indexInChunk;
                }

                rec.archetype = dstArch;
                rec.chunkIndex = newChunkIndex;
                rec.indexInChunk = newIndexInChunk;

                // 初期化関数で値を取得してコンポーネントを初期化
                T* comp = dstArch->GetComponent<T>(newChunkIndex, newIndexInChunk);
                T value = initializer(idx, actor);
                new (comp) T(std::move(value));

                ++successCount;
            }
        }

        return successCount;
    }

    //------------------------------------------------------------------------
    //! @brief 複数Actorからコンポーネントを一括削除
    //! @tparam T コンポーネント型
    //! @param actors 対象Actor配列
    //! @return 削除されたコンポーネント数
    //!
    //! @code
    //! world.RemoveComponentsBatch<VelocityData>(stoppedActors);
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    size_t RemoveComponentsBatch(const std::vector<Actor>& actors) {
        ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT();

        if (actors.empty()) return 0;

        auto& registry = container_.ECS().GetActorRegistry();
        auto& archetypes = container_.ECS().GetArchetypeStorage();
        size_t removedCount = 0;

        // 元Archetypeでグループ化
        std::unordered_map<Archetype*, std::vector<Actor>> byArchetype;
        for (Actor actor : actors) {
            if (!registry.IsAlive(actor)) continue;
            ActorRecord& rec = registry.GetRecord(actor);
            if (rec.archetype && rec.archetype->HasComponent<T>()) {
                byArchetype[rec.archetype].push_back(actor);
            }
        }

        for (auto& [srcArch, group] : byArchetype) {
            Archetype* dstArch = archetypes.GetOrCreateWithout<T>(srcArch);

            for (Actor actor : group) {
                ActorRecord& rec = registry.GetRecord(actor);

                Actor swappedActor;
                auto [newChunkIndex, newIndexInChunk] = dstArch->MoveActorFrom(
                    srcArch, rec.chunkIndex, rec.indexInChunk, actor, swappedActor);

                if (swappedActor.IsValid()) {
                    ActorRecord& swappedRec = registry.GetRecord(swappedActor);
                    swappedRec.indexInChunk = rec.indexInChunk;
                }

                rec.archetype = dstArch;
                rec.chunkIndex = newChunkIndex;
                rec.indexInChunk = newIndexInChunk;

                ++removedCount;
            }
        }

        return removedCount;
    }

    //========================================================================
    // バッチクエリ
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 単一コンポーネントに対してイテレーション (SoA対応)
    //! @tparam T コンポーネントの型
    //! @tparam Func 処理関数の型
    //! @param func 各アクターとコンポーネントに対して呼び出す関数
    //!
    //! @note In<T>/Out<T>/InOut<T>を使う場合は型安全な新APIを使用してください
    //------------------------------------------------------------------------
    template<typename T, typename Func,
             typename = std::enable_if_t<!is_access_mode_v<T>>>
    void ForEach(Func&& func) {
        container_.ECS().GetArchetypeStorage().ForEachMatching<T>([&func](Archetype& arch) {
            const auto& metas = arch.GetChunkMetas();
            for (size_t ci = 0; ci < metas.size(); ++ci) {
                const Actor* actors = arch.GetActorArray(ci);
                T* compArray = arch.GetComponentArray<T>(ci);  // SoA: 連続配列
                for (uint16_t i = 0; i < metas[ci].count; ++i) {
                    func(actors[i], compArray[i]);  // 連続アクセス
                }
            }
        });
    }

    //------------------------------------------------------------------------
    //! @brief 2つのコンポーネントを持つアクターに対してイテレーション (SoA対応)
    //! @tparam T1, T2 コンポーネントの型
    //! @tparam Func 処理関数の型
    //! @param func 各アクターとコンポーネントに対して呼び出す関数
    //!
    //! @note In<T>/Out<T>/InOut<T>を使う場合は型安全な新APIを使用してください
    //------------------------------------------------------------------------
    template<typename T1, typename T2, typename Func,
             typename = std::enable_if_t<!is_access_mode_v<T1> && !is_access_mode_v<T2>>>
    void ForEach(Func&& func) {
        container_.ECS().GetArchetypeStorage().ForEachMatching<T1, T2>([&func](Archetype& arch) {
            const auto& metas = arch.GetChunkMetas();
            for (size_t ci = 0; ci < metas.size(); ++ci) {
                const Actor* actors = arch.GetActorArray(ci);
                T1* array1 = arch.GetComponentArray<T1>(ci);  // SoA: 連続配列
                T2* array2 = arch.GetComponentArray<T2>(ci);
                for (uint16_t i = 0; i < metas[ci].count; ++i) {
                    func(actors[i], array1[i], array2[i]);  // 連続アクセス
                }
            }
        });
    }

    //------------------------------------------------------------------------
    //! @brief 3つのコンポーネントを持つアクターに対してイテレーション (SoA対応)
    //! @tparam T1, T2, T3 コンポーネントの型
    //! @tparam Func 処理関数の型
    //! @param func 各アクターとコンポーネントに対して呼び出す関数
    //!
    //! @note In<T>/Out<T>/InOut<T>を使う場合は型安全な新APIを使用してください
    //------------------------------------------------------------------------
    template<typename T1, typename T2, typename T3, typename Func,
             typename = std::enable_if_t<!is_access_mode_v<T1> && !is_access_mode_v<T2> && !is_access_mode_v<T3>>>
    void ForEach(Func&& func) {
        container_.ECS().GetArchetypeStorage().ForEachMatching<T1, T2, T3>([&func](Archetype& arch) {
            const auto& metas = arch.GetChunkMetas();
            for (size_t ci = 0; ci < metas.size(); ++ci) {
                const Actor* actors = arch.GetActorArray(ci);
                T1* array1 = arch.GetComponentArray<T1>(ci);  // SoA: 連続配列
                T2* array2 = arch.GetComponentArray<T2>(ci);
                T3* array3 = arch.GetComponentArray<T3>(ci);
                for (uint16_t i = 0; i < metas[ci].count; ++i) {
                    func(actors[i], array1[i], array2[i], array3[i]);  // 連続アクセス
                }
            }
        });
    }

    //========================================================================
    // const ForEach (読み取り専用イテレーション)
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 単一コンポーネントに対してconst イテレーション (SoA対応)
    //------------------------------------------------------------------------
    template<typename T, typename Func,
             typename = std::enable_if_t<!is_access_mode_v<T>>>
    void ForEach(Func&& func) const {
        container_.ECS().GetArchetypeStorage().ForEachMatching<T>([&func](const Archetype& arch) {
            const auto& metas = arch.GetChunkMetas();
            for (size_t ci = 0; ci < metas.size(); ++ci) {
                const Actor* actors = arch.GetActorArray(ci);
                const T* compArray = arch.GetComponentArray<T>(ci);  // SoA: 連続配列
                for (uint16_t i = 0; i < metas[ci].count; ++i) {
                    func(actors[i], compArray[i]);  // 連続アクセス
                }
            }
        });
    }

    //------------------------------------------------------------------------
    //! @brief 2つのコンポーネントに対してconst イテレーション (SoA対応)
    //------------------------------------------------------------------------
    template<typename T1, typename T2, typename Func,
             typename = std::enable_if_t<!is_access_mode_v<T1> && !is_access_mode_v<T2>>>
    void ForEach(Func&& func) const {
        container_.ECS().GetArchetypeStorage().ForEachMatching<T1, T2>([&func](const Archetype& arch) {
            const auto& metas = arch.GetChunkMetas();
            for (size_t ci = 0; ci < metas.size(); ++ci) {
                const Actor* actors = arch.GetActorArray(ci);
                const T1* array1 = arch.GetComponentArray<T1>(ci);  // SoA: 連続配列
                const T2* array2 = arch.GetComponentArray<T2>(ci);
                for (uint16_t i = 0; i < metas[ci].count; ++i) {
                    func(actors[i], array1[i], array2[i]);  // 連続アクセス
                }
            }
        });
    }

    //------------------------------------------------------------------------
    //! @brief 3つのコンポーネントに対してconst イテレーション (SoA対応)
    //------------------------------------------------------------------------
    template<typename T1, typename T2, typename T3, typename Func,
             typename = std::enable_if_t<!is_access_mode_v<T1> && !is_access_mode_v<T2> && !is_access_mode_v<T3>>>
    void ForEach(Func&& func) const {
        container_.ECS().GetArchetypeStorage().ForEachMatching<T1, T2, T3>([&func](const Archetype& arch) {
            const auto& metas = arch.GetChunkMetas();
            for (size_t ci = 0; ci < metas.size(); ++ci) {
                const Actor* actors = arch.GetActorArray(ci);
                const T1* array1 = arch.GetComponentArray<T1>(ci);  // SoA: 連続配列
                const T2* array2 = arch.GetComponentArray<T2>(ci);
                const T3* array3 = arch.GetComponentArray<T3>(ci);
                for (uint16_t i = 0; i < metas[ci].count; ++i) {
                    func(actors[i], array1[i], array2[i], array3[i]);  // 連続アクセス
                }
            }
        });
    }

    //========================================================================
    // 型安全な ForEach (In/Out/InOut対応)
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief In/Out/InOut対応の型安全なForEach
    //!
    //! アクセスモード（In<T>/Out<T>/InOut<T>）を指定してForEachを実行。
    //! コンパイル時にラムダの引数型が正しいか検証される。
    //!
    //! - In<T>: 読み取り専用。ラムダには const T& として渡される。
    //! - Out<T>: 書き込み専用。ラムダには T& として渡される。
    //! - InOut<T>: 読み書き両方。ラムダには T& として渡される。
    //!
    //! @tparam AccessModes In<T>/Out<T>/InOut<T>のシーケンス
    //! @tparam Func ラムダ型（Actor, 各コンポーネント参照を受け取る）
    //! @param func 各アクターに対して呼び出す関数
    //!
    //! @code
    //! // 読み書き + 読み取り専用の例
    //! world.ForEach<InOut<TransformData>, In<VelocityData>>(
    //!     [dt](Actor e, TransformData& t, const VelocityData& v) {
    //!         t.position += v.velocity * dt;
    //!     });
    //!
    //! // 読み取り専用のみ
    //! world.ForEach<In<TransformData>>(
    //!     [](Actor e, const TransformData& t) {
    //!         // t を読み取るのみ
    //!     });
    //!
    //! // 書き込み専用（初期化など）
    //! world.ForEach<Out<DamageData>>(
    //!     [](Actor e, DamageData& d) {
    //!         d.value = 0;
    //!     });
    //! @endcode
    //------------------------------------------------------------------------
    template<typename... AccessModes, typename Func,
             typename = std::enable_if_t<detail::all_are_access_modes_v<AccessModes...>>>
    void ForEach(Func&& func) {
        // 全てがアクセスモード（In/Out/InOut）であることを検証
        static_assert(detail::all_are_access_modes_v<AccessModes...>,
            "All type parameters must be In<T>, Out<T>, or InOut<T>");

        // コンポーネント数制限（1〜8）
        static_assert(sizeof...(AccessModes) >= 1 && sizeof...(AccessModes) <= 8,
            "ForEach supports 1 to 8 access modes");

        // ラムダの引数型がアクセスモードと一致することを検証
        static_assert(detail::validate_lambda_args_v<Func, AccessModes...>,
            "Lambda argument types must match access modes: "
            "In<T> requires const T&, Out<T>/InOut<T> requires T&");

        // 変更追跡用のバージョンを設定
        container_.ECS().GetArchetypeStorage().SetWriteVersion(container_.GetFrameCount());

        detail::TypedForEachImpl<AccessModes...>(container_.ECS().GetArchetypeStorage(), std::forward<Func>(func));
    }

    //------------------------------------------------------------------------
    //! @brief 型安全なクエリを構築
    //! @tparam Ts 必須コンポーネント型群
    //! @return Query<Ts...>オブジェクト
    //!
    //! 使用例:
    //! ```cpp
    //! world.Query<TransformData, SpriteData>().ForEach([](Actor e, auto& t, auto& s) {
    //!     // 処理
    //! });
    //! ```
    //------------------------------------------------------------------------
    template<typename... Ts>
    [[nodiscard]] Query<Ts...> Query() {
        return ECS::Query<Ts...>(this);
    }

    //------------------------------------------------------------------------
    //! @brief キャッシュ付きクエリを作成
    //! @tparam Ts コンポーネント型群
    //! @return CachedQueryオブジェクト
    //!
    //! Archetypeマッチング結果をキャッシュし、毎フレームの再検索を回避。
    //! シーン初期化時に一度作成し、毎フレーム使い回すことでパフォーマンス向上。
    //!
    //! 使用例:
    //! ```cpp
    //! // シーン初期化時
    //! auto query = world.CreateCachedQuery<TransformData, SpriteData>();
    //!
    //! // 毎フレーム
    //! query.ForEach([](Actor e, auto& t, auto& s) { ... });
    //! ```
    //------------------------------------------------------------------------
    template<typename... Ts>
    [[nodiscard]] CachedQuery<Ts...> CreateCachedQuery() {
        return CachedQuery<Ts...>(this);
    }

    //========================================================================
    // 並列バッチクエリ（JobSystem統合）
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 任意数のコンポーネントに対して並列イテレーション（レガシー版）
    //! @tparam Ts コンポーネント型群（1つ以上）
    //! @tparam Func 処理関数の型 void(Actor, Ts&...)
    //! @param func 各アクターとコンポーネントに対して呼び出す関数
    //! @return JobHandle 完了待機用ハンドル
    //!
    //! @note Chunk単位で並列実行。ラムダ内でのActor生成/破棄は禁止。
    //!       コンポーネント追加/削除はDeferred版を使用すること。
    //!
    //! @deprecated アクセスモード版 ParallelForEach を推奨
    //------------------------------------------------------------------------
    template<typename... Ts, typename Func,
             std::enable_if_t<!detail::all_are_access_modes_v<Ts...>, int> = 0>
    [[nodiscard]] JobHandle ParallelForEach(Func&& func);

    //------------------------------------------------------------------------
    //! @brief アクセスモード付き並列イテレーション（型安全版）
    //! @tparam AccessModes In<T>/Out<T>/InOut<T>の組み合わせ
    //! @tparam Func 処理関数の型 void(Actor, ...)
    //! @param func 各アクターとコンポーネントに対して呼び出す関数
    //! @return JobHandle 完了待機用ハンドル
    //!
    //! In<T>: 読み取り専用（const T&）- 他の並列ジョブと同時読み取り可能
    //! Out<T>: 書き込み専用（T&）- 排他的アクセス
    //! InOut<T>: 読み書き両用（T&）- 排他的アクセス
    //!
    //! コンパイル時に重複コンポーネントアクセスを検出。
    //!
    //! @code
    //! // 位置を更新（速度は読み取りのみ）
    //! auto handle = world.ParallelForEach<InOut<TransformData>, In<VelocityData>>(
    //!     [dt](Actor e, TransformData& t, const VelocityData& v) {
    //!         t.position += v.velocity * dt;
    //!     });
    //! handle.Wait();
    //! @endcode
    //------------------------------------------------------------------------
    template<typename... AccessModes, typename Func,
             std::enable_if_t<detail::all_are_access_modes_v<AccessModes...>, int> = 0>
    [[nodiscard]] JobHandle ParallelForEach(Func&& func);

    //========================================================================
    // System管理
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 更新Systemをクラスベースで登録（シンプル版）
    //! @tparam T ISystemを継承したクラス（finalを推奨）
    //!
    //! 依存関係なしで即座に登録。依存関係指定が必要な場合は
    //! RegisterSystemWithDeps<T>().After<U>() を使用。
    //------------------------------------------------------------------------
    template<typename T>
    void RegisterSystem() {
        static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from ISystem");
        auto system = std::make_unique<T>();

        // SystemEntryを構築
        SystemEntry entry;
        entry.id = std::type_index(typeid(T));
        entry.priority = system->Priority();
        entry.name = system->Name();
        entry.system = std::move(system);

        // グラフに追加してソート
        CommitSystem(std::move(entry));
    }

    //------------------------------------------------------------------------
    //! @brief 描画Systemをクラスベースで登録（シンプル版）
    //! @tparam T IRenderSystemを継承したクラス（finalを推奨）
    //------------------------------------------------------------------------
    template<typename T>
    void RegisterRenderSystem() {
        static_assert(std::is_base_of_v<IRenderSystem, T>, "T must inherit from IRenderSystem");
        auto system = std::make_unique<T>();

        // RenderSystemEntryを構築
        RenderSystemEntry entry;
        entry.id = std::type_index(typeid(T));
        entry.priority = system->Priority();
        entry.name = system->Name();
        entry.system = std::move(system);

        // グラフに追加してソート
        CommitRenderSystem(std::move(entry));
    }

    //------------------------------------------------------------------------
    //! @brief SystemEntryを直接登録（SystemBuilderから呼ばれる）
    //! @param entry System登録情報
    //------------------------------------------------------------------------
    void CommitSystem(SystemEntry entry);

    //------------------------------------------------------------------------
    //! @brief RenderSystemEntryを直接登録（RenderSystemBuilderから呼ばれる）
    //! @param entry RenderSystem登録情報
    //------------------------------------------------------------------------
    void CommitRenderSystem(RenderSystemEntry entry);

    //========================================================================
    // フレーム制御
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief フレーム開始処理
    //!
    //! 遅延操作を一括実行し、フレームカウンターをインクリメント。
    //! この関数の呼び出し後、次のBeginFrame()までポインタが安定する。
    //------------------------------------------------------------------------
    void BeginFrame();

    //------------------------------------------------------------------------
    //! @brief フレーム終了処理
    //!
    //! 現在は特に処理なし（将来の拡張用）
    //------------------------------------------------------------------------
    void EndFrame();

    //------------------------------------------------------------------------
    //! @brief フレームカウンターを取得
    //! @return 現在のフレームカウンター
    //------------------------------------------------------------------------
    [[nodiscard]] uint32_t GetFrameCounter() const noexcept {
        return container_.GetFrameCount();
    }

    //========================================================================
    // フレーム処理
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 固定タイムステップ更新
    //! @param dt 固定デルタタイム（通常1/60秒）
    //------------------------------------------------------------------------
    void FixedUpdate(float dt) {
        // 遅延破棄を処理
        ProcessDeferredDestroys(dt);

        if (systemsDirty_) {
            RebuildSortedSystems();
        }
        for (ISystem* system : sortedSystems_) {
            system->OnUpdate(*this, dt);
        }
    }

    //------------------------------------------------------------------------
    //! @brief 描画
    //! @param alpha 補間係数（0.0〜1.0）
    //------------------------------------------------------------------------
    void Render(float alpha) {
        if (renderSystemsDirty_) {
            RebuildSortedRenderSystems();
        }
        for (IRenderSystem* renderSystem : sortedRenderSystems_) {
            renderSystem->OnRender(*this, alpha);
        }
    }

    //========================================================================
    // OOP API（従来のGameObjectベース）
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief ゲームオブジェクトを生成
    //! @param name オブジェクト名
    //! @return 生成されたゲームオブジェクトへのポインタ
    //------------------------------------------------------------------------
    GameObject* CreateGameObject(const std::string& name = "GameObject");

    //------------------------------------------------------------------------
    //! @brief ゲームオブジェクトを破棄
    //! @param go 破棄するゲームオブジェクト
    //------------------------------------------------------------------------
    void DestroyGameObject(GameObject* go);

    //------------------------------------------------------------------------
    //! @brief 名前でゲームオブジェクトを検索
    //! @param name 検索する名前
    //! @return 見つかったゲームオブジェクト（見つからない場合はnullptr）
    //------------------------------------------------------------------------
    [[nodiscard]] GameObject* FindGameObject(const std::string& name);

    //========================================================================
    // ユーティリティ
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 全アクターとコンポーネントをクリア
    //------------------------------------------------------------------------
    void Clear();

    //------------------------------------------------------------------------
    //! @brief Systemもクリア
    //------------------------------------------------------------------------
    void ClearAll();

    //========================================================================
    // Transform便利API
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief Actorのワールド行列を取得（オンデマンド計算）
    //! @param actor 対象のアクター
    //! @return ワールド行列（LocalToWorldDataがあればその値、なければ計算）
    //!
    //! LocalToWorldDataを持っていない場合でも、Position/Rotation/Scaleから
    //! その場で計算して返す。
    //!
    //! @code
    //! Matrix worldMat = world.GetWorldMatrix(actor);
    //! Vector3 worldPos = worldMat.Translation();
    //! @endcode
    //------------------------------------------------------------------------
    [[nodiscard]] Matrix GetWorldMatrix(Actor actor) const;

    //========================================================================
    // 遅延破棄API
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 指定時間後にActorを破棄
    //! @param actor 破棄対象
    //! @param delaySeconds 遅延時間（秒）
    //!
    //! @code
    //! // 弾丸を5秒後に自動破棄
    //! world.DestroyAfter(bullet, 5.0f);
    //!
    //! // パーティクルを2秒後に破棄
    //! world.DestroyAfter(particle, 2.0f);
    //! @endcode
    //------------------------------------------------------------------------
    void DestroyAfter(Actor actor, float delaySeconds);

    //------------------------------------------------------------------------
    //! @brief 遅延破棄をキャンセル
    //! @param actor キャンセル対象
    //! @return キャンセルできたらtrue
    //------------------------------------------------------------------------
    bool CancelDestroyAfter(Actor actor);

private:
    //------------------------------------------------------------------------
    //! @brief Systemのソート済み配列を再構築
    //------------------------------------------------------------------------
    void RebuildSortedSystems();

    //------------------------------------------------------------------------
    //! @brief RenderSystemのソート済み配列を再構築
    //------------------------------------------------------------------------
    void RebuildSortedRenderSystems();

private:
    //========================================================================
    // コンテナ
    //========================================================================

    //! 統合コンテナ（ECS/GameObject/System管理）
    WorldContainer container_;

    //========================================================================
    // System管理（WorldContainerのSystemSchedulerとは別に管理）
    //========================================================================

    //! System依存関係グラフ（IDと依存関係のみ）
    SystemGraph systemGraph_;
    RenderSystemGraph renderSystemGraph_;

    //! System実体をIDで管理
    std::unordered_map<SystemId, std::unique_ptr<ISystem>> systemsById_;
    std::unordered_map<SystemId, std::unique_ptr<IRenderSystem>> renderSystemsById_;

    //! ソート済みSystem配列（実行用、生ポインタ）
    std::vector<ISystem*> sortedSystems_;
    std::vector<IRenderSystem*> sortedRenderSystems_;

    //! ソート済み配列の再構築フラグ
    bool systemsDirty_ = false;
    bool renderSystemsDirty_ = false;

    //========================================================================
    // 遅延破棄
    //========================================================================
    struct DeferredDestroy {
        Actor actor;
        float remainingTime;
    };
    std::vector<DeferredDestroy> deferredDestroys_;

    //! @brief 遅延破棄を処理（BeginFrameで呼び出し）
    void ProcessDeferredDestroys(float dt);
};

} // namespace ECS

// DeferredQueue::PushAdd/PushRemoveの実装（Worldの完全定義が必要なため末尾でインクルード）
#include "detail/deferred_queue_impl.h"

// DeferredContext テンプレート実装
#include "detail/deferred_context_impl.h"

// CachedQuery テンプレート実装
#include "detail/cached_query_impl.h"

// ParallelForEachの実装（JobSystem統合）
#include "detail/parallel_foreach_impl.h"

// EntityCommandBuffer テンプレート実装
#include "detail/entity_command_buffer_impl.h"

// ComponentRef テンプレート実装
#include "detail/component_ref_impl.h"
