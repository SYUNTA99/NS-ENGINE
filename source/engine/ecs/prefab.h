//----------------------------------------------------------------------------
//! @file   prefab.h
//! @brief  ECS Prefab - エンティティテンプレート
//----------------------------------------------------------------------------
#pragma once


#include "common/stl/stl_common.h"
#include "common/stl/stl_metaprogramming.h"
#include "archetype.h"
#include "archetype_storage.h"

namespace ECS {

class World;

//============================================================================
//! @brief Prefabデータ（内部用）
//============================================================================
struct PrefabData {
    Archetype* archetype = nullptr;           //!< 対応するArchetype
    std::vector<std::byte> componentData;     //!< 全コンポーネントの連結データ
    size_t componentDataSize = 0;             //!< 1Actorあたりのデータサイズ

    [[nodiscard]] bool IsValid() const noexcept {
        return archetype != nullptr;
    }
};

//============================================================================
//! @brief Prefab - 確定済みエンティティテンプレート
//!
//! コンポーネント構成を保存し、高速にインスタンス化可能。
//! Archetypeを事前確定することでAddComponent連発を回避。
//!
//! @code
//! // 初期化時にPrefab定義
//! auto bulletPrefab = world.CreatePrefab()
//!     .Add<TransformData>(Vector3::Zero)
//!     .Add<VelocityData>(Vector3::Zero)
//!     .Build();
//!
//! // ゲームループ中に高速生成
//! Actor bullet = world.Instantiate(bulletPrefab);
//! world.GetComponent<TransformData>(bullet)->position = spawnPos;
//! @endcode
//============================================================================
class Prefab {
public:
    Prefab() = default;
    ~Prefab() = default;

    // ムーブのみ許可
    Prefab(Prefab&&) = default;
    Prefab& operator=(Prefab&&) = default;
    Prefab(const Prefab&) = delete;
    Prefab& operator=(const Prefab&) = delete;

    [[nodiscard]] bool IsValid() const noexcept {
        return data_.IsValid();
    }

    [[nodiscard]] Archetype* GetArchetype() const noexcept {
        return data_.archetype;
    }

    [[nodiscard]] const std::byte* GetComponentData() const noexcept {
        return data_.componentData.data();
    }

    [[nodiscard]] size_t GetComponentDataSize() const noexcept {
        return data_.componentDataSize;
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネントのオフセットを取得
    //! @tparam T コンポーネント型
    //! @return Prefabデータ内でのオフセット（存在しない場合はSIZE_MAX）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] size_t GetComponentOffset() const noexcept {
        if (!data_.archetype) return SIZE_MAX;
        return data_.archetype->GetComponentOffset<T>();
    }

    //------------------------------------------------------------------------
    //! @brief Prefabデータ内のコンポーネント順序オフセットを取得
    //! @param compIdx コンポーネントインデックス
    //! @return Prefab内でのシーケンシャルオフセット
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetPrefabComponentOffset(size_t compIdx) const noexcept {
        if (!data_.archetype) return SIZE_MAX;
        const auto& components = data_.archetype->GetComponents();
        if (compIdx >= components.size()) return SIZE_MAX;

        // Prefab内はシーケンシャル配置
        size_t offset = 0;
        for (size_t i = 0; i < compIdx; ++i) {
            offset += components[i].size;
        }
        return offset;
    }

    //------------------------------------------------------------------------
    //! @brief PrefabデータをArchetypeのSoA配列にコピー
    //! @param arch 対象Archetype
    //! @param chunkIndex Chunkインデックス
    //! @param indexInChunk Chunk内のエンティティインデックス
    //------------------------------------------------------------------------
    void CopyComponentsTo(Archetype* arch, uint32_t chunkIndex, uint16_t indexInChunk) const {
        if (!arch || data_.componentData.empty()) return;

        const auto& components = arch->GetComponents();
        const std::byte* prefabData = data_.componentData.data();
        size_t prefabOffset = 0;

        for (size_t compIdx = 0; compIdx < components.size(); ++compIdx) {
            const auto& info = components[compIdx];
            if (info.size == 0) continue;  // Tagコンポーネントはスキップ

            // SoA: 各コンポーネント配列の対象位置にコピー
            std::byte* chunkBase = arch->GetChunk(chunkIndex)->Data();
            std::byte* dstArrayBase = chunkBase + info.offset;
            std::byte* dst = dstArrayBase + static_cast<size_t>(indexInChunk) * info.size;

            std::memcpy(dst, prefabData + prefabOffset, info.size);
            prefabOffset += info.size;
        }
    }

private:
    friend class PrefabBuilder;
    friend class World;

    PrefabData data_;
};

//============================================================================
//! @brief PrefabBuilder - Prefabの構築API
//!
//! チェーンメソッドでコンポーネントを追加し、Build()で確定。
//!
//! @code
//! auto prefab = PrefabBuilder(storage)
//!     .Add<TransformData>(Vector3::Zero, Quaternion::Identity, Vector3::One)
//!     .Add<VelocityData>(Vector3::Zero)
//!     .Build();
//! @endcode
//============================================================================
class PrefabBuilder {
public:
    explicit PrefabBuilder(ArchetypeStorage& storage)
        : storage_(storage) {}

    //------------------------------------------------------------------------
    //! @brief コンポーネントを追加（初期値付き）
    //! @tparam T コンポーネント型
    //! @tparam Args コンストラクタ引数
    //! @param args 初期値を構築するコンストラクタ引数
    //! @return *this（チェーン呼び出し用）
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    PrefabBuilder& Add(Args&&... args) {
        static_assert(std::is_trivially_copyable_v<T>,
            "ECS components must be trivially copyable");

        // ComponentInfoを追加
        constexpr size_t size = is_tag_component_v<T> ? 0 : sizeof(T);
        constexpr size_t align = is_tag_component_v<T> ? 1 : alignof(T);
        ComponentInfo info(
            std::type_index(typeid(T)),
            size,
            align
        );
        componentInfos_.push_back(info);

        // 初期値を構築して保存（Tagコンポーネントはデータなし）
        if constexpr (!is_tag_component_v<T>) {
            ComponentValue value;
            value.typeIndex = std::type_index(typeid(T));
            value.data.resize(sizeof(T));
            new (value.data.data()) T(std::forward<Args>(args)...);
            componentValues_.push_back(std::move(value));
        }

        return *this;
    }

    //------------------------------------------------------------------------
    //! @brief Prefabを確定（Build後のBuilderは再利用不可）
    //! @return 確定済みPrefab
    //------------------------------------------------------------------------
    [[nodiscard]] Prefab Build() {
        Prefab prefab;

        if (componentInfos_.empty()) {
            return prefab;
        }

        // 1. Archetypeを取得または作成
        prefab.data_.archetype = storage_.GetOrCreate(componentInfos_);

        // 2. componentDataSizeを取得（1エンティティあたりの合計サイズ）
        prefab.data_.componentDataSize = prefab.data_.archetype->GetComponentDataSize();

        // 3. データバッファを確保
        if (prefab.data_.componentDataSize > 0) {
            prefab.data_.componentData.resize(prefab.data_.componentDataSize);
            std::memset(prefab.data_.componentData.data(), 0, prefab.data_.componentDataSize);

            // 4. 各コンポーネントの初期値をシーケンシャルにコピー
            // Prefab内はシーケンシャル配置（SoAオフセットではなく順番に並べる）
            const auto& components = prefab.data_.archetype->GetComponents();
            size_t sequentialOffset = 0;

            for (size_t compIdx = 0; compIdx < components.size(); ++compIdx) {
                const auto& info = components[compIdx];
                if (info.size == 0) continue;  // Tagコンポーネントはスキップ

                // この型の初期値を探す
                for (const auto& value : componentValues_) {
                    if (value.typeIndex == info.type) {
                        std::memcpy(
                            prefab.data_.componentData.data() + sequentialOffset,
                            value.data.data(),
                            info.size
                        );
                        break;
                    }
                }
                sequentialOffset += info.size;
            }
        }

        return prefab;
    }

private:
    struct ComponentValue {
        std::type_index typeIndex = std::type_index(typeid(void));
        std::vector<std::byte> data;
    };

    ArchetypeStorage& storage_;
    std::vector<ComponentInfo> componentInfos_;
    std::vector<ComponentValue> componentValues_;
};

} // namespace ECS
