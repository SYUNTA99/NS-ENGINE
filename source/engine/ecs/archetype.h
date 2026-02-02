//----------------------------------------------------------------------------
//! @file   archetype.h
//! @brief  ECS Archetype - コンポーネント構成の定義とChunk管理
//----------------------------------------------------------------------------
#pragma once

#include "actor.h"
#include "chunk.h"
#include "common/utility/non_copyable.h"
#include <vector>
#include <typeindex>
#include <algorithm>
#include <memory>
#include <cassert>
#include <cstring>

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
    size_t offset;  //!< コンポーネントデータ内のオフセット

    ComponentInfo(std::type_index t, size_t s, size_t a, size_t o = 0)
        : type(t), size(s), alignment(a), offset(o) {}

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
//! Chunk内メモリレイアウト:
//! [Actor0][Actor1]...[ActorN] | [Comp0_data][Comp1_data]...[CompN_data]
//! <---- Actor配列 -------->   <---- コンポーネントデータ ------------>
//============================================================================
class Archetype : private NonCopyable {
public:
    //------------------------------------------------------------------------
    //! @brief Chunk毎のメタデータ
    //------------------------------------------------------------------------
    struct ChunkMeta {
        uint16_t count = 0;  //!< このChunk内のActor数
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
    [[nodiscard]] size_t GetComponentDataSize() const noexcept { return componentDataSize_; }
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

    template<typename T>
    [[nodiscard]] size_t GetComponentOffset() const noexcept {
        const ComponentInfo* info = GetComponentInfo<T>();
        return info ? info->offset : SIZE_MAX;
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
    //! @brief Chunk内のコンポーネントデータベースへのポインタを取得
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
    //! @brief 指定位置のコンポーネントデータへのポインタを取得
    //------------------------------------------------------------------------
    [[nodiscard]] void* GetComponentAt(uint32_t chunkIndex, uint16_t indexInChunk, size_t componentOffset) noexcept {
        assert(chunkIndex < chunks_.size());
        assert(indexInChunk < chunkMetas_[chunkIndex].count);
        std::byte* base = GetComponentDataBase(chunkIndex);
        return base + static_cast<size_t>(indexInChunk) * componentDataSize_ + componentOffset;
    }

    [[nodiscard]] const void* GetComponentAt(uint32_t chunkIndex, uint16_t indexInChunk, size_t componentOffset) const noexcept {
        assert(chunkIndex < chunks_.size());
        assert(indexInChunk < chunkMetas_[chunkIndex].count);
        const std::byte* base = GetComponentDataBase(chunkIndex);
        return base + static_cast<size_t>(indexInChunk) * componentDataSize_ + componentOffset;
    }

    //------------------------------------------------------------------------
    //! @brief 指定位置のコンポーネントを取得
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponent(uint32_t chunkIndex, uint16_t indexInChunk) noexcept {
        const ComponentInfo* info = GetComponentInfo<T>();
        if (!info || chunkIndex >= chunks_.size()) return nullptr;
        return static_cast<T*>(GetComponentAt(chunkIndex, indexInChunk, info->offset));
    }

    template<typename T>
    [[nodiscard]] const T* GetComponent(uint32_t chunkIndex, uint16_t indexInChunk) const noexcept {
        const ComponentInfo* info = GetComponentInfo<T>();
        if (!info || chunkIndex >= chunks_.size()) return nullptr;
        return static_cast<const T*>(GetComponentAt(chunkIndex, indexInChunk, info->offset));
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
        outChunkIndex = static_cast<uint32_t>(chunks_.size() - 1);
        outIndexInChunk = chunkMetas_[outChunkIndex].count++;
        Actor* actors = GetActorArray(outChunkIndex);
        actors[outIndexInChunk] = actor;
        return true;
    }

    //------------------------------------------------------------------------
    //! @brief Actorを解放（swap-and-pop）
    //! @return 移動が発生した場合、移動元のインデックス。移動なしなら UINT16_MAX
    //------------------------------------------------------------------------
    uint16_t DeallocateActor(uint32_t chunkIndex, uint16_t indexInChunk) {
        assert(chunkIndex < chunks_.size());
        ChunkMeta& meta = chunkMetas_[chunkIndex];
        assert(indexInChunk < meta.count);
        assert(meta.count > 0);

        uint16_t lastIndex = meta.count - 1;
        --meta.count;

        if (indexInChunk != lastIndex) {
            // swap-and-pop: 末尾のデータを削除位置にコピー
            Actor* actors = GetActorArray(chunkIndex);
            actors[indexInChunk] = actors[lastIndex];

            // コンポーネントデータもコピー
            if (componentDataSize_ > 0) {
                std::byte* base = GetComponentDataBase(chunkIndex);
                std::byte* dst = base + static_cast<size_t>(indexInChunk) * componentDataSize_;
                const std::byte* src = base + static_cast<size_t>(lastIndex) * componentDataSize_;
                std::memcpy(dst, src, componentDataSize_);
            }
            return lastIndex;
        }

        return UINT16_MAX;
    }

    //------------------------------------------------------------------------
    //! @brief 他のArchetypeからActorを移動
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

        // 2. sourceからデータをコピー（共通コンポーネントのみ）
        if (source && source != this) {
            // 共通コンポーネントを探してコピー
            for (const auto& dstInfo : components_) {
                const ComponentInfo* srcInfo = source->GetComponentInfo(dstInfo.type);
                if (srcInfo) {
                    const void* srcPtr = source->GetComponentAt(srcChunkIndex, srcIndexInChunk, srcInfo->offset);
                    void* dstPtr = GetComponentAt(dstChunkIndex, dstIndexInChunk, dstInfo.offset);
                    std::memcpy(dstPtr, srcPtr, dstInfo.size);
                }
            }

            // 3. sourceから削除（swap-and-pop）
            uint16_t swappedFrom = source->DeallocateActor(srcChunkIndex, srcIndexInChunk);
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
    //! @brief レイアウト計算
    //------------------------------------------------------------------------
    void CalculateLayout() {
        // 1. コンポーネントデータサイズとオフセットを計算
        size_t offset = 0;
        size_t maxAlign = 1;

        for (auto& info : components_) {
            size_t alignment = info.alignment;
            maxAlign = (std::max)(maxAlign, alignment);
            offset = (offset + alignment - 1) & ~(alignment - 1);
            info.offset = offset;
            offset += info.size;
        }

        // コンポーネントデータサイズ（アラインメント考慮）
        componentDataSize_ = components_.empty() ? 0 : ((offset + maxAlign - 1) & ~(maxAlign - 1));

        // 2. Chunk容量を計算
        // 1Actor = sizeof(Actor) + componentDataSize_
        size_t perActorSize = sizeof(Actor) + componentDataSize_;

        if (perActorSize > 0) {
            chunkCapacity_ = static_cast<uint16_t>(Chunk::kSize / perActorSize);
        } else {
            // 空Archetype（コンポーネントなし）
            chunkCapacity_ = static_cast<uint16_t>(Chunk::kSize / sizeof(Actor));
        }

        // 最低1つは格納できるように
        if (chunkCapacity_ == 0) {
            chunkCapacity_ = 1;
        }

        // 3. コンポーネントデータ開始オフセット
        // Actor配列の直後、アラインメント考慮
        size_t actorArraySize = static_cast<size_t>(chunkCapacity_) * sizeof(Actor);
        componentDataOffset_ = (actorArraySize + maxAlign - 1) & ~(maxAlign - 1);
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
        components_.emplace_back(
            std::type_index(typeid(T)),
            sizeof(T),
            alignof(T)
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
