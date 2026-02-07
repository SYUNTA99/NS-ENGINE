//----------------------------------------------------------------------------
//! @file   archetype.h
//! @brief  ECS Archetype - コンポーネント構成の定義とChunk管理
//----------------------------------------------------------------------------
#pragma once


#include "common/stl/stl_common.h"
#include "common/stl/stl_containers.h"
#include "common/stl/stl_metaprogramming.h"
#include "common/utility/non_copyable.h"
#include "actor.h"
#include "chunk.h"
#include "component_data.h"
#include "buffer/buffer_element.h"
#include "buffer/buffer_header.h"
#include "buffer/internal_buffer_capacity.h"
#include "buffer/dynamic_buffer.h"

namespace ECS {

//============================================================================
//! @brief ArchetypeId
//============================================================================
using ArchetypeId = size_t;
static constexpr ArchetypeId kInvalidArchetypeId = 0;

//============================================================================
//! @brief コンポーネント情報
//============================================================================
struct ComponentInfo {
    std::type_index type;
    size_t size;
    size_t alignment;
    size_t offset;          //!< コンポーネントデータ内のオフセット
    bool isBuffer;          //!< DynamicBufferコンポーネントか
    size_t elementSize;     //!< バッファ要素のサイズ（isBuffer時のみ有効）
    int32_t inlineCapacity; //!< インライン容量（isBuffer時のみ有効）

    ComponentInfo(std::type_index t, size_t s, size_t a, size_t o = 0)
        : type(t), size(s), alignment(a), offset(o)
        , isBuffer(false), elementSize(0), inlineCapacity(0) {}

    //! @brief バッファコンポーネント用コンストラクタ
    ComponentInfo(std::type_index t, size_t s, size_t a,
                  size_t elemSize, int32_t inlineCap)
        : type(t), size(s), alignment(a), offset(0)
        , isBuffer(true), elementSize(elemSize), inlineCapacity(inlineCap) {}

    bool operator<(const ComponentInfo& other) const {
        return type < other.type;
    }
};

//============================================================================
//! @brief Archetype
//!
//! 同じコンポーネント構成を持つActorの集合を管理する。
//! Chunkは純粋な16KBバッファで、メタデータはArchetypeが管理。
//!
//! Chunk内メモリレイアウト（SoA - Structure of Arrays）:
//! [Actor0,Actor1,...,ActorN] | [Pos0,Pos1,...] | [Vel0,Vel1,...] | ...
//! <---- Actor配列 -------->   <-- Comp0配列 --> <-- Comp1配列 -->
//!
//! 各コンポーネント型が連続配置されることで、ForEach時のキャッシュ効率が向上。
//============================================================================
class Archetype : private NonCopyable {
public:
    //------------------------------------------------------------------------
    //! @brief Chunk毎のメタデータ
    //------------------------------------------------------------------------
    struct ChunkMeta {
        uint16_t count = 0;  //!< このChunk内のActor数

        //! コンポーネント別の変更バージョン
        //! インデックスはArchetype::components_のインデックスに対応
        //! 値はそのコンポーネントが最後に書き込まれたバージョン
        std::vector<uint32_t> componentVersions;

        //! コンポーネント別の有効ビット（Enableable Component用）
        //! enabledBits[compIndex][entityIndex / 64] のビット(entityIndex % 64)
        //! 1 = 有効, 0 = 無効
        //! デフォルトは全ビット1（全て有効）
        std::vector<std::vector<uint64_t>> enabledBits;

        //! @brief コンポーネントバージョン配列を初期化
        void InitVersions(size_t componentCount) {
            componentVersions.resize(componentCount, 0);
        }

        //! @brief 有効ビット配列を初期化（デフォルト全有効）
        void InitEnabledBits(size_t componentCount, uint16_t capacity) {
            size_t wordsPerComp = (capacity + 63) / 64;
            enabledBits.resize(componentCount);
            for (auto& bits : enabledBits) {
                bits.resize(wordsPerComp, ~0ULL);  // 全ビット1（全有効）
            }
        }

        //! @brief 指定位置の有効ビットを取得
        [[nodiscard]] bool IsEnabled(size_t compIndex, uint16_t entityIndex) const {
            if (compIndex >= enabledBits.size()) return true;  // 未初期化は有効扱い
            size_t wordIdx = entityIndex / 64;
            size_t bitIdx = entityIndex % 64;
            if (wordIdx >= enabledBits[compIndex].size()) return true;
            return (enabledBits[compIndex][wordIdx] & (1ULL << bitIdx)) != 0;
        }

        //! @brief 指定位置の有効ビットを設定
        void SetEnabled(size_t compIndex, uint16_t entityIndex, bool enabled) {
            if (compIndex >= enabledBits.size()) return;
            size_t wordIdx = entityIndex / 64;
            size_t bitIdx = entityIndex % 64;
            if (wordIdx >= enabledBits[compIndex].size()) return;
            if (enabled) {
                enabledBits[compIndex][wordIdx] |= (1ULL << bitIdx);
            } else {
                enabledBits[compIndex][wordIdx] &= ~(1ULL << bitIdx);
            }
        }
    };

    //------------------------------------------------------------------------
    // メモリ確保演算子
    //------------------------------------------------------------------------
    [[nodiscard]] static void* operator new(size_t size) {
        if (Memory::MemorySystem::Get().IsInitialized()) {
            return Memory::GetDefaultAllocator().Allocate(size, alignof(Archetype));
        }
        return ::operator new(size);
    }

    static void operator delete(void* ptr) noexcept {
        if (!ptr) return;
        if (Memory::MemorySystem::Get().IsInitialized()) {
            Memory::GetDefaultAllocator().Deallocate(ptr, sizeof(Archetype));
            return;
        }
        ::operator delete(ptr);
    }

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------

    //! @brief デフォルトコンストラクタ（空のArchetype）
    Archetype() {
        CalculateLayout();
    }

    //! @brief コンポーネント情報からArchetypeを構築
    explicit Archetype(std::vector<ComponentInfo> components)
        : components_(std::move(components))
    {
        CalculateLayout();
        id_ = CalculateId();
    }

    //------------------------------------------------------------------------
    // アクセサ
    //------------------------------------------------------------------------

    [[nodiscard]] ArchetypeId GetId() const noexcept { return id_; }
    [[nodiscard]] const std::vector<ComponentInfo>& GetComponents() const noexcept { return components_; }
    [[nodiscard]] uint16_t GetChunkCapacity() const noexcept { return chunkCapacity_; }

    [[nodiscard]] size_t GetActorCount() const noexcept {
        size_t count = 0;
        for (const auto& meta : chunkMetas_) {
            count += meta.count;
        }
        return count;
    }

    [[nodiscard]] size_t GetChunkCount() const noexcept { return chunks_.size(); }

    //! Chunk配列への参照（イテレーション用）
    [[nodiscard]] std::vector<std::unique_ptr<Chunk>>& GetChunks() noexcept { return chunks_; }
    [[nodiscard]] const std::vector<std::unique_ptr<Chunk>>& GetChunks() const noexcept { return chunks_; }

    //! ChunkMeta配列への参照
    [[nodiscard]] std::vector<ChunkMeta>& GetChunkMetas() noexcept { return chunkMetas_; }
    [[nodiscard]] const std::vector<ChunkMeta>& GetChunkMetas() const noexcept { return chunkMetas_; }

    [[nodiscard]] Chunk* GetChunk(size_t index) noexcept {
        return index < chunks_.size() ? chunks_[index].get() : nullptr;
    }

    [[nodiscard]] const Chunk* GetChunk(size_t index) const noexcept {
        return index < chunks_.size() ? chunks_[index].get() : nullptr;
    }

    [[nodiscard]] uint16_t GetChunkActorCount(size_t chunkIndex) const noexcept {
        return chunkIndex < chunkMetas_.size() ? chunkMetas_[chunkIndex].count : 0;
    }

    //------------------------------------------------------------------------
    //! @brief 指定型のコンポーネント情報を取得
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] const ComponentInfo* GetComponentInfo() const noexcept {
        std::type_index typeIdx = std::type_index(typeid(T));
        for (const auto& info : components_) {
            if (info.type == typeIdx) return &info;
        }
        return nullptr;
    }

    [[nodiscard]] const ComponentInfo* GetComponentInfo(std::type_index typeIdx) const noexcept {
        for (const auto& info : components_) {
            if (info.type == typeIdx) return &info;
        }
        return nullptr;
    }

    template<typename T>
    [[nodiscard]] bool HasComponent() const noexcept {
        return GetComponentInfo<T>() != nullptr;
    }

    //! @brief 型インデックスでコンポーネントを所持しているか確認
    //! @param typeIdx コンポーネントの型インデックス
    //! @return 所持している場合はtrue
    [[nodiscard]] bool HasComponentByTypeIndex(std::type_index typeIdx) const noexcept {
        for (const auto& info : components_) {
            if (info.type == typeIdx) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    [[nodiscard]] size_t GetComponentOffset() const noexcept {
        const ComponentInfo* info = GetComponentInfo<T>();
        return info ? info->offset : SIZE_MAX;
    }

    //------------------------------------------------------------------------
    //! @brief 1Actorあたりのコンポーネントデータサイズを取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetComponentDataSize() const noexcept {
        return componentDataSize_;
    }

    //------------------------------------------------------------------------
    //! @brief Chunk内コンポーネントデータ開始位置を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetComponentDataOffset() const noexcept {
        return componentDataOffset_;
    }

    //------------------------------------------------------------------------
    //! @brief Chunk内のコンポーネントデータベースアドレスを取得
    //! @param chunkIndex Chunkインデックス
    //! @return コンポーネントデータの先頭アドレス
    //------------------------------------------------------------------------
    [[nodiscard]] std::byte* GetComponentDataBase(size_t chunkIndex) noexcept {
        if (chunkIndex >= chunks_.size()) return nullptr;
        return chunks_[chunkIndex]->Data() + componentDataOffset_;
    }

    [[nodiscard]] const std::byte* GetComponentDataBase(size_t chunkIndex) const noexcept {
        if (chunkIndex >= chunks_.size()) return nullptr;
        return chunks_[chunkIndex]->Data() + componentDataOffset_;
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントのインデックスを取得
    //! @tparam T コンポーネント型
    //! @return コンポーネントのインデックス（存在しない場合はSIZE_MAX）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] size_t GetComponentIndex() const noexcept {
        std::type_index typeIdx = std::type_index(typeid(T));
        for (size_t i = 0; i < components_.size(); ++i) {
            if (components_[i].type == typeIdx) return i;
        }
        return SIZE_MAX;
    }

    [[nodiscard]] size_t GetComponentIndex(std::type_index typeIdx) const noexcept {
        for (size_t i = 0; i < components_.size(); ++i) {
            if (components_[i].type == typeIdx) return i;
        }
        return SIZE_MAX;
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントの有効状態を取得
    //! @tparam T コンポーネント型
    //! @param chunkIndex Chunkインデックス
    //! @param indexInChunk Chunk内のインデックス
    //! @return 有効ならtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool IsComponentEnabled(uint32_t chunkIndex, uint16_t indexInChunk) const {
        size_t compIdx = GetComponentIndex<T>();
        if (compIdx == SIZE_MAX || chunkIndex >= chunkMetas_.size()) return true;
        return chunkMetas_[chunkIndex].IsEnabled(compIdx, indexInChunk);
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントの有効状態を設定
    //! @tparam T コンポーネント型
    //! @param chunkIndex Chunkインデックス
    //! @param indexInChunk Chunk内のインデックス
    //! @param enabled 有効にするか
    //------------------------------------------------------------------------
    template<typename T>
    void SetComponentEnabled(uint32_t chunkIndex, uint16_t indexInChunk, bool enabled) {
        size_t compIdx = GetComponentIndex<T>();
        if (compIdx == SIZE_MAX || chunkIndex >= chunkMetas_.size()) return;
        chunkMetas_[chunkIndex].SetEnabled(compIdx, indexInChunk, enabled);
    }

    //------------------------------------------------------------------------
    //! @brief Chunkのコンポーネントバージョンを更新（書き込み時に呼ぶ）
    //! @tparam T コンポーネント型
    //! @param chunkIndex Chunkインデックス
    //! @param version 新しいバージョン番号
    //------------------------------------------------------------------------
    template<typename T>
    void MarkComponentWritten(size_t chunkIndex, uint32_t version) {
        size_t compIdx = GetComponentIndex<T>();
        if (compIdx != SIZE_MAX && chunkIndex < chunkMetas_.size()) {
            ChunkMeta& meta = chunkMetas_[chunkIndex];
            if (compIdx < meta.componentVersions.size()) {
                meta.componentVersions[compIdx] = version;
            }
        }
    }

    void MarkComponentWritten(size_t chunkIndex, size_t compIdx, uint32_t version) {
        if (chunkIndex < chunkMetas_.size() && compIdx < chunkMetas_[chunkIndex].componentVersions.size()) {
            chunkMetas_[chunkIndex].componentVersions[compIdx] = version;
        }
    }

    //------------------------------------------------------------------------
    //! @brief Chunkのコンポーネントバージョンを取得
    //! @tparam T コンポーネント型
    //! @param chunkIndex Chunkインデックス
    //! @return バージョン番号（0は未書き込み）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] uint32_t GetComponentVersion(size_t chunkIndex) const noexcept {
        size_t compIdx = GetComponentIndex<T>();
        if (compIdx != SIZE_MAX && chunkIndex < chunkMetas_.size()) {
            const ChunkMeta& meta = chunkMetas_[chunkIndex];
            if (compIdx < meta.componentVersions.size()) {
                return meta.componentVersions[compIdx];
            }
        }
        return 0;
    }

    [[nodiscard]] uint32_t GetComponentVersion(size_t chunkIndex, size_t compIdx) const noexcept {
        if (chunkIndex < chunkMetas_.size() && compIdx < chunkMetas_[chunkIndex].componentVersions.size()) {
            return chunkMetas_[chunkIndex].componentVersions[compIdx];
        }
        return 0;
    }

    //------------------------------------------------------------------------
    //! @brief Chunk内のActor配列へのポインタを取得
    //------------------------------------------------------------------------
    [[nodiscard]] Actor* GetActorArray(size_t chunkIndex) noexcept {
        if (chunkIndex >= chunks_.size()) return nullptr;
        return reinterpret_cast<Actor*>(chunks_[chunkIndex]->Data());
    }

    [[nodiscard]] const Actor* GetActorArray(size_t chunkIndex) const noexcept {
        if (chunkIndex >= chunks_.size()) return nullptr;
        return reinterpret_cast<const Actor*>(chunks_[chunkIndex]->Data());
    }

    //------------------------------------------------------------------------
    //! @brief Chunk内の指定コンポーネント配列へのポインタを取得（SoA）
    //! @tparam T コンポーネント型
    //! @param chunkIndex Chunkインデックス
    //! @return コンポーネント配列の先頭ポインタ
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponentArray(size_t chunkIndex) noexcept {
        size_t compIdx = GetComponentIndex<T>();
        if (compIdx == SIZE_MAX || chunkIndex >= chunks_.size()) return nullptr;
        return reinterpret_cast<T*>(chunks_[chunkIndex]->Data() + components_[compIdx].offset);
    }

    template<typename T>
    [[nodiscard]] const T* GetComponentArray(size_t chunkIndex) const noexcept {
        size_t compIdx = GetComponentIndex<T>();
        if (compIdx == SIZE_MAX || chunkIndex >= chunks_.size()) return nullptr;
        return reinterpret_cast<const T*>(chunks_[chunkIndex]->Data() + components_[compIdx].offset);
    }

    //------------------------------------------------------------------------
    //! @brief Chunk内の指定インデックスのコンポーネント配列へのポインタを取得
    //! @param chunkIndex Chunkインデックス
    //! @param compIdx コンポーネントインデックス
    //! @return コンポーネント配列の先頭ポインタ
    //------------------------------------------------------------------------
    [[nodiscard]] std::byte* GetComponentArrayByIndex(size_t chunkIndex, size_t compIdx) noexcept {
        if (compIdx >= components_.size() || chunkIndex >= chunks_.size()) return nullptr;
        return chunks_[chunkIndex]->Data() + components_[compIdx].offset;
    }

    [[nodiscard]] const std::byte* GetComponentArrayByIndex(size_t chunkIndex, size_t compIdx) const noexcept {
        if (compIdx >= components_.size() || chunkIndex >= chunks_.size()) return nullptr;
        return chunks_[chunkIndex]->Data() + components_[compIdx].offset;
    }

    //------------------------------------------------------------------------
    //! @brief 指定位置のActorを取得
    //------------------------------------------------------------------------
    [[nodiscard]] Actor GetActorAt(uint32_t chunkIndex, uint16_t indexInChunk) const {
        assert(chunkIndex < chunks_.size());
        assert(indexInChunk < chunkMetas_[chunkIndex].count);
        const Actor* actors = GetActorArray(chunkIndex);
        return actors[indexInChunk];
    }

    //------------------------------------------------------------------------
    //! @brief 指定位置にActorを設定
    //------------------------------------------------------------------------
    void SetActorAt(uint32_t chunkIndex, uint16_t indexInChunk, Actor actor) {
        assert(chunkIndex < chunks_.size());
        assert(indexInChunk < chunkMetas_[chunkIndex].count);
        Actor* actors = GetActorArray(chunkIndex);
        actors[indexInChunk] = actor;
    }

    //------------------------------------------------------------------------
    //! @brief 指定位置のコンポーネントデータへのポインタを取得（SoA）
    //! @param chunkIndex Chunkインデックス
    //! @param indexInChunk Chunk内インデックス
    //! @param compIdx コンポーネントインデックス
    //! @return コンポーネントデータへのポインタ
    //------------------------------------------------------------------------
    [[nodiscard]] void* GetComponentAt(uint32_t chunkIndex, uint16_t indexInChunk, size_t compIdx) noexcept {
        assert(chunkIndex < chunks_.size());
        assert(indexInChunk < chunkMetas_[chunkIndex].count);
        assert(compIdx < components_.size());
        std::byte* arrayBase = chunks_[chunkIndex]->Data() + components_[compIdx].offset;
        return arrayBase + static_cast<size_t>(indexInChunk) * components_[compIdx].size;
    }

    [[nodiscard]] const void* GetComponentAt(uint32_t chunkIndex, uint16_t indexInChunk, size_t compIdx) const noexcept {
        assert(chunkIndex < chunks_.size());
        assert(indexInChunk < chunkMetas_[chunkIndex].count);
        assert(compIdx < components_.size());
        const std::byte* arrayBase = chunks_[chunkIndex]->Data() + components_[compIdx].offset;
        return arrayBase + static_cast<size_t>(indexInChunk) * components_[compIdx].size;
    }

    //------------------------------------------------------------------------
    //! @brief 指定位置のコンポーネントを取得（SoA）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponent(uint32_t chunkIndex, uint16_t indexInChunk) noexcept {
        T* array = GetComponentArray<T>(chunkIndex);
        if (!array) return nullptr;
        return &array[indexInChunk];
    }

    template<typename T>
    [[nodiscard]] const T* GetComponent(uint32_t chunkIndex, uint16_t indexInChunk) const noexcept {
        const T* array = GetComponentArray<T>(chunkIndex);
        if (!array) return nullptr;
        return &array[indexInChunk];
    }

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを所持しているか確認
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @return バッファを所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool HasBuffer() const noexcept {
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");
        std::type_index typeIdx = std::type_index(typeid(T));
        for (const auto& info : components_) {
            if (info.type == typeIdx && info.isBuffer) {
                return true;
            }
        }
        return false;
    }

    //------------------------------------------------------------------------
    //! @brief 指定位置のDynamicBufferを取得（SoA対応）
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param chunkIndex Chunkインデックス
    //! @param indexInChunk Chunk内のインデックス
    //! @return DynamicBuffer<T>アクセサ（存在しない場合は無効なアクセサ）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] DynamicBuffer<T> GetBuffer(uint32_t chunkIndex, uint16_t indexInChunk) noexcept {
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        std::type_index typeIdx = std::type_index(typeid(T));
        for (size_t compIdx = 0; compIdx < components_.size(); ++compIdx) {
            const auto& info = components_[compIdx];
            if (info.type == typeIdx && info.isBuffer) {
                // SoA: GetComponentAtにコンポーネントインデックスを渡す
                std::byte* base = static_cast<std::byte*>(
                    GetComponentAt(chunkIndex, indexInChunk, compIdx));
                BufferHeader* header = reinterpret_cast<BufferHeader*>(base);
                std::byte* inlineData = base + sizeof(BufferHeader);
                return DynamicBuffer<T>(header, inlineData);
            }
        }
        return DynamicBuffer<T>();
    }

    //------------------------------------------------------------------------
    //! @brief DynamicBufferを取得（const版）
    //! @tparam T バッファ要素型（IBufferElement継承）
    //! @param chunkIndex Chunkインデックス
    //! @param indexInChunk Chunk内のインデックス
    //! @return ConstDynamicBuffer<T>アクセサ（存在しない場合は無効なアクセサ）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] ConstDynamicBuffer<T> GetConstBuffer(uint32_t chunkIndex, uint16_t indexInChunk) const noexcept {
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        std::type_index typeIdx = std::type_index(typeid(T));
        for (size_t compIdx = 0; compIdx < components_.size(); ++compIdx) {
            const auto& info = components_[compIdx];
            if (info.type == typeIdx && info.isBuffer) {
                // SoA: GetComponentAtにコンポーネントインデックスを渡す
                const std::byte* base = static_cast<const std::byte*>(
                    GetComponentAt(chunkIndex, indexInChunk, compIdx));
                const BufferHeader* header = reinterpret_cast<const BufferHeader*>(base);
                const std::byte* inlineData = base + sizeof(BufferHeader);
                return ConstDynamicBuffer<T>(header, inlineData);
            }
        }
        return ConstDynamicBuffer<T>();
    }

    //------------------------------------------------------------------------
    //! @brief 新しいActorのためのスロットを確保
    //------------------------------------------------------------------------
    bool AllocateActor(Actor actor, uint32_t& outChunkIndex, uint16_t& outIndexInChunk) {
        // 空きのあるChunkを探す
        for (size_t i = 0; i < chunks_.size(); ++i) {
            if (chunkMetas_[i].count < chunkCapacity_) {
                outChunkIndex = static_cast<uint32_t>(i);
                outIndexInChunk = chunkMetas_[i].count++;
                Actor* actors = GetActorArray(i);
                actors[outIndexInChunk] = actor;
                return true;
            }
        }

        // 新しいChunkを作成
        chunks_.push_back(std::make_unique<Chunk>());
        chunkMetas_.push_back(ChunkMeta{});
        chunkMetas_.back().InitVersions(components_.size());
        chunkMetas_.back().InitEnabledBits(components_.size(), chunkCapacity_);
        outChunkIndex = static_cast<uint32_t>(chunks_.size() - 1);
        outIndexInChunk = chunkMetas_[outChunkIndex].count++;
        Actor* actors = GetActorArray(outChunkIndex);
        actors[outIndexInChunk] = actor;
        return true;
    }

    //------------------------------------------------------------------------
    //! @brief 複数Actorのスロットを一括確保
    //! @param actors 確保するActor配列
    //! @param outPositions 出力: (chunkIndex, indexInChunk)のペア配列
    //------------------------------------------------------------------------
    void AllocateActors(
        const std::vector<Actor>& actors,
        std::vector<std::pair<uint32_t, uint16_t>>& outPositions)
    {
        outPositions.clear();
        outPositions.reserve(actors.size());

        size_t actorIdx = 0;
        const size_t totalActors = actors.size();

        // 既存Chunkの空きスロットを埋める
        for (size_t ci = 0; ci < chunks_.size() && actorIdx < totalActors; ++ci) {
            ChunkMeta& meta = chunkMetas_[ci];
            Actor* actorArray = GetActorArray(ci);

            while (meta.count < chunkCapacity_ && actorIdx < totalActors) {
                uint16_t indexInChunk = meta.count++;
                actorArray[indexInChunk] = actors[actorIdx];
                outPositions.emplace_back(static_cast<uint32_t>(ci), indexInChunk);
                ++actorIdx;
            }
        }

        // 残りは新しいChunkを作成して割り当て
        while (actorIdx < totalActors) {
            chunks_.push_back(std::make_unique<Chunk>());
            chunkMetas_.push_back(ChunkMeta{});
            chunkMetas_.back().InitVersions(components_.size());
            chunkMetas_.back().InitEnabledBits(components_.size(), chunkCapacity_);
            uint32_t chunkIndex = static_cast<uint32_t>(chunks_.size() - 1);
            ChunkMeta& meta = chunkMetas_[chunkIndex];
            Actor* actorArray = GetActorArray(chunkIndex);

            while (meta.count < chunkCapacity_ && actorIdx < totalActors) {
                uint16_t indexInChunk = meta.count++;
                actorArray[indexInChunk] = actors[actorIdx];
                outPositions.emplace_back(chunkIndex, indexInChunk);
                ++actorIdx;
            }
        }
    }

    //------------------------------------------------------------------------
    //! @brief Actorを解放（swap-and-pop、SoA対応）
    //! @return 移動が発生した場合、移動元のインデックス。移動なしなら UINT16_MAX
    //------------------------------------------------------------------------
    uint16_t DeallocateActor(uint32_t chunkIndex, uint16_t indexInChunk) {
        assert(chunkIndex < chunks_.size());
        ChunkMeta& meta = chunkMetas_[chunkIndex];
        assert(indexInChunk < meta.count);
        assert(meta.count > 0);

        // 削除対象のバッファをクリーンアップ
        CleanupBuffers(chunkIndex, indexInChunk);

        uint16_t lastIndex = meta.count - 1;
        --meta.count;

        if (indexInChunk != lastIndex) {
            // swap-and-pop: 末尾のデータを削除位置にコピー
            Actor* actors = GetActorArray(chunkIndex);
            actors[indexInChunk] = actors[lastIndex];

            // SoA: 各コンポーネント配列を個別にswap
            std::byte* chunkBase = chunks_[chunkIndex]->Data();
            for (size_t c = 0; c < components_.size(); ++c) {
                const auto& info = components_[c];
                if (info.size == 0) continue;  // Tagコンポーネントはスキップ

                std::byte* arrayBase = chunkBase + info.offset;
                std::byte* dst = arrayBase + static_cast<size_t>(indexInChunk) * info.size;
                const std::byte* src = arrayBase + static_cast<size_t>(lastIndex) * info.size;
                std::memcpy(dst, src, info.size);
            }
            return lastIndex;
        }

        return UINT16_MAX;
    }

    //------------------------------------------------------------------------
    //! @brief 指定位置のバッファコンポーネントの外部ストレージを解放（SoA対応）
    //! @param chunkIndex Chunkインデックス
    //! @param indexInChunk Chunk内のインデックス
    //------------------------------------------------------------------------
    void CleanupBuffers(uint32_t chunkIndex, uint16_t indexInChunk) {
        for (size_t compIdx = 0; compIdx < components_.size(); ++compIdx) {
            const auto& info = components_[compIdx];
            if (info.isBuffer && info.elementSize > 0) {
                BufferHeader* header = static_cast<BufferHeader*>(
                    GetComponentAt(chunkIndex, indexInChunk, compIdx));

                if (header->externalPtr) {
                    Memory::GetDefaultAllocator().Deallocate(
                        header->externalPtr,
                        static_cast<size_t>(header->externalCapacity) * info.elementSize);
                    header->externalPtr = nullptr;
                    header->length = 0;
                }
            }
        }
    }

    //------------------------------------------------------------------------
    //! @brief Actorを解放（バッファクリーンアップなし、SoA対応）
    //!
    //! MoveActorFromで外部ストレージを移譲済みの場合に使用。
    //! @return 移動が発生した場合、移動元のインデックス。移動なしなら UINT16_MAX
    //------------------------------------------------------------------------
    uint16_t DeallocateActorWithoutBufferCleanup(uint32_t chunkIndex, uint16_t indexInChunk) {
        assert(chunkIndex < chunks_.size());
        ChunkMeta& meta = chunkMetas_[chunkIndex];
        assert(indexInChunk < meta.count);
        assert(meta.count > 0);

        uint16_t lastIndex = meta.count - 1;
        --meta.count;

        if (indexInChunk != lastIndex) {
            Actor* actors = GetActorArray(chunkIndex);
            actors[indexInChunk] = actors[lastIndex];

            // SoA: 各コンポーネント配列を個別にswap
            std::byte* chunkBase = chunks_[chunkIndex]->Data();
            for (size_t c = 0; c < components_.size(); ++c) {
                const auto& info = components_[c];
                if (info.size == 0) continue;  // Tagコンポーネントはスキップ

                std::byte* arrayBase = chunkBase + info.offset;
                std::byte* dst = arrayBase + static_cast<size_t>(indexInChunk) * info.size;
                const std::byte* src = arrayBase + static_cast<size_t>(lastIndex) * info.size;
                std::memcpy(dst, src, info.size);
            }
            return lastIndex;
        }

        return UINT16_MAX;
    }

private:
    //------------------------------------------------------------------------
    //! @brief バッファコンポーネントを移動（外部ストレージの所有権を移譲、SoA対応）
    //------------------------------------------------------------------------
    void MoveBuffer(
        Archetype* source,
        uint32_t srcChunkIndex, uint16_t srcIndexInChunk,
        size_t srcCompIdx,
        uint32_t dstChunkIndex, uint16_t dstIndexInChunk,
        size_t dstCompIdx)
    {
        const ComponentInfo& srcInfo = source->components_[srcCompIdx];
        const ComponentInfo& dstInfo = components_[dstCompIdx];

        // ソースのヘッダーとインラインデータを取得
        std::byte* srcBase = static_cast<std::byte*>(
            source->GetComponentAt(srcChunkIndex, srcIndexInChunk, srcCompIdx));
        BufferHeader* srcHeader = reinterpret_cast<BufferHeader*>(srcBase);
        std::byte* srcInlineData = srcBase + sizeof(BufferHeader);

        // デスティネーションのヘッダーとインラインデータを取得
        std::byte* dstBase = static_cast<std::byte*>(
            GetComponentAt(dstChunkIndex, dstIndexInChunk, dstCompIdx));
        BufferHeader* dstHeader = reinterpret_cast<BufferHeader*>(dstBase);
        std::byte* dstInlineData = dstBase + sizeof(BufferHeader);

        // ヘッダーをコピー
        *dstHeader = *srcHeader;
        dstHeader->inlineCapacity = dstInfo.inlineCapacity;

        if (srcHeader->externalPtr) {
            // 外部ストレージ: 所有権を移譲（ポインタは既にコピー済み）
            // ソース側のポインタをクリアして二重解放を防止
            srcHeader->externalPtr = nullptr;
            srcHeader->length = 0;
        } else {
            // インラインデータ: memcpyでコピー
            size_t copySize = static_cast<size_t>(srcHeader->length) * srcInfo.elementSize;
            if (copySize > 0) {
                std::memcpy(dstInlineData, srcInlineData, copySize);
            }
        }
    }

public:

    //------------------------------------------------------------------------
    //! @brief 他のArchetypeからActorを移動（SoA対応）
    //------------------------------------------------------------------------
    std::pair<uint32_t, uint16_t> MoveActorFrom(
        Archetype* source,
        uint32_t srcChunkIndex,
        uint16_t srcIndexInChunk,
        Actor actor,
        Actor& outSwappedActor)
    {
        // 1. このArchetypeにスロットを確保
        uint32_t dstChunkIndex;
        uint16_t dstIndexInChunk;
        AllocateActor(actor, dstChunkIndex, dstIndexInChunk);

        // 2. sourceからデータをコピー（共通コンポーネントのみ、SoA）
        if (source && source != this) {
            // 共通コンポーネントを探してコピー
            for (size_t dstCompIdx = 0; dstCompIdx < components_.size(); ++dstCompIdx) {
                const auto& dstInfo = components_[dstCompIdx];
                size_t srcCompIdx = source->GetComponentIndex(dstInfo.type);
                if (srcCompIdx != SIZE_MAX) {
                    const auto& srcInfo = source->components_[srcCompIdx];

                    if (dstInfo.isBuffer && srcInfo.isBuffer) {
                        // バッファコンポーネント: 特殊処理
                        MoveBuffer(source, srcChunkIndex, srcIndexInChunk, srcCompIdx,
                                   dstChunkIndex, dstIndexInChunk, dstCompIdx);
                    } else {
                        // 通常コンポーネント: memcpy（SoAアクセス）
                        void* dstPtr = GetComponentAt(dstChunkIndex, dstIndexInChunk, dstCompIdx);
                        const void* srcPtr = source->GetComponentAt(srcChunkIndex, srcIndexInChunk, srcCompIdx);
                        std::memcpy(dstPtr, srcPtr, dstInfo.size);
                    }
                }
            }

            // 3. sourceから削除（swap-and-pop）
            // 注意: バッファの外部ストレージは移譲済みなのでCleanupBuffersはスキップ
            uint16_t swappedFrom = source->DeallocateActorWithoutBufferCleanup(srcChunkIndex, srcIndexInChunk);
            if (swappedFrom != UINT16_MAX) {
                outSwappedActor = source->GetActorAt(srcChunkIndex, srcIndexInChunk);
            } else {
                outSwappedActor = Actor();
            }
        } else {
            outSwappedActor = Actor();
        }

        return {dstChunkIndex, dstIndexInChunk};
    }

    //------------------------------------------------------------------------
    // ArchetypeId計算
    //------------------------------------------------------------------------
    static ArchetypeId CalculateId(const std::vector<ComponentInfo>& components) {
        if (components.empty()) return kInvalidArchetypeId;
        size_t hash = 14695981039346656037ull;
        for (const auto& info : components) {
            hash ^= info.type.hash_code();
            hash *= 1099511628211ull;
        }
        return hash;
    }

    template<typename... Ts>
    static ArchetypeId CalculateId() {
        std::vector<std::type_index> types = { std::type_index(typeid(Ts))... };
        std::sort(types.begin(), types.end());
        size_t hash = 14695981039346656037ull;
        for (const auto& type : types) {
            hash ^= type.hash_code();
            hash *= 1099511628211ull;
        }
        return hash;
    }

private:
    //------------------------------------------------------------------------
    //! @brief レイアウト計算（SoA - Structure of Arrays）
    //!
    //! Chunk内レイアウト:
    //! [Actor0, Actor1, ...] | [Comp0_0, Comp0_1, ...] | [Comp1_0, Comp1_1, ...] | ...
    //!
    //! 各コンポーネント型が連続配置されるため、ForEach時のキャッシュ効率が向上。
    //------------------------------------------------------------------------
    void CalculateLayout() {
        if (components_.empty()) {
            // 空Archetype（コンポーネントなし）
            chunkCapacity_ = static_cast<uint16_t>(Chunk::kSize / sizeof(Actor));
            if (chunkCapacity_ == 0) chunkCapacity_ = 1;
            componentDataSize_ = 0;
            componentDataOffset_ = 0;
            return;
        }

        // 1. 1エンティティあたりの合計サイズを計算（容量計算用）
        size_t totalComponentSize = 0;
        size_t maxAlign = 1;
        for (const auto& info : components_) {
            totalComponentSize += info.size;
            maxAlign = (std::max)(maxAlign, info.alignment);
        }

        // 2. Chunk容量を計算
        // perActorSize = Actor + 全コンポーネント合計
        size_t perActorSize = sizeof(Actor) + totalComponentSize;
        chunkCapacity_ = static_cast<uint16_t>(Chunk::kSize / perActorSize);
        if (chunkCapacity_ == 0) {
            chunkCapacity_ = 1;
        }

        // 3. SoAレイアウト: 各コンポーネント配列のオフセットを計算
        // Actor配列の直後から開始
        size_t actorArraySize = static_cast<size_t>(chunkCapacity_) * sizeof(Actor);
        size_t currentOffset = (actorArraySize + maxAlign - 1) & ~(maxAlign - 1);
        componentDataOffset_ = currentOffset;

        for (auto& info : components_) {
            // アラインメント調整
            currentOffset = (currentOffset + info.alignment - 1) & ~(info.alignment - 1);
            info.offset = currentOffset;  // この配列の開始オフセット
            // 配列全体のサイズを加算
            currentOffset += info.size * static_cast<size_t>(chunkCapacity_);
        }

        // componentDataSize_は互換性のため保持（1エンティティあたり）
        componentDataSize_ = totalComponentSize;
    }

    [[nodiscard]] ArchetypeId CalculateId() const {
        return CalculateId(components_);
    }

private:
    ArchetypeId id_ = kInvalidArchetypeId;
    std::vector<ComponentInfo> components_;
    std::vector<std::unique_ptr<Chunk>> chunks_;
    std::vector<ChunkMeta> chunkMetas_;      //!< Chunk毎のメタデータ

    size_t componentDataSize_ = 0;           //!< 1Actorのコンポーネントデータサイズ
    size_t componentDataOffset_ = 0;         //!< Chunk内コンポーネントデータ開始位置
    uint16_t chunkCapacity_ = 0;             //!< 1Chunk当たりの最大Actor数
};

//============================================================================
//! @brief Archetypeビルダー
//============================================================================
class ArchetypeBuilder {
public:
    template<typename T>
    ArchetypeBuilder& Add() {
        // Tagコンポーネントはサイズ0として扱う（メモリを消費しない）
        constexpr size_t size = is_tag_component_v<T> ? 0 : sizeof(T);
        constexpr size_t align = is_tag_component_v<T> ? 1 : alignof(T);
        components_.emplace_back(
            std::type_index(typeid(T)),
            size,
            align
        );
        return *this;
    }

    //------------------------------------------------------------------------
    //! @brief DynamicBufferコンポーネントを追加
    //!
    //! @tparam T Buffer要素型（IBufferElement継承）
    //!
    //! BufferHeader + InlineDataのスロットを確保する。
    //! インライン容量はInternalBufferCapacity<T>::Valueで決定。
    //------------------------------------------------------------------------
    template<typename T>
    ArchetypeBuilder& AddBuffer() {
        static_assert(is_buffer_element_v<T>,
            "T must inherit from IBufferElement and be trivially_copyable");

        constexpr int32_t inlineCap = InternalBufferCapacity<T>::Value;
        constexpr size_t totalSize = sizeof(BufferHeader) +
                                     static_cast<size_t>(inlineCap) * sizeof(T);

        // DynamicBuffer<T>の型情報を使用（実際にはBufferHeader+Dataが格納される）
        components_.emplace_back(
            std::type_index(typeid(T)),  // 要素型で識別
            totalSize,
            alignof(BufferHeader),
            sizeof(T),   // elementSize
            inlineCap    // inlineCapacity
        );
        return *this;
    }

    [[nodiscard]] std::unique_ptr<Archetype> Build() {
        std::sort(components_.begin(), components_.end());
        return std::make_unique<Archetype>(std::move(components_));
    }

    [[nodiscard]] ArchetypeId CalculateId() const {
        auto sorted = components_;
        std::sort(sorted.begin(), sorted.end());
        return Archetype::CalculateId(sorted);
    }

private:
    std::vector<ComponentInfo> components_;
};

} // namespace ECS
