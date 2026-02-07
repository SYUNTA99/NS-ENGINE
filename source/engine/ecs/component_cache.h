//----------------------------------------------------------------------------
//! @file   component_cache.h
//! @brief  ECS ComponentCache - フレーム単位のコンポーネントキャッシュ
//----------------------------------------------------------------------------
#pragma once


#include "actor.h"
#include <array>
#include <unordered_map>
#include <typeindex>
#include <cstdint>

namespace ECS {

// 前方宣言
class World;

//============================================================================
//! @brief ComponentCache
//!
//! GameObjectでコンポーネントポインタをフレーム単位でキャッシュする。
//! フレーム内では同じコンポーネントへのアクセスが直接ポインタ返却となり、
//! 2回目以降のアクセスが ~1ns で完了する。
//!
//! 仕組み:
//! - 各キャッシュエントリはポインタとフレーム番号を保持
//! - フレームカウンターが変わるとキャッシュミスとなり再取得
//! - 最大8型まで高速パス、それ以上はオーバーフローマップを使用
//!
//! メモリ: 最大8型 × 16バイト = 128バイト + オーバーフローマップ
//============================================================================
class ComponentCache {
public:
    //! 高速パスの最大エントリ数
    static constexpr size_t kFastPathSize = 8;

    ComponentCache() = default;
    ~ComponentCache() = default;

    // コピー・ムーブ許可
    ComponentCache(const ComponentCache&) = default;
    ComponentCache& operator=(const ComponentCache&) = default;
    ComponentCache(ComponentCache&&) = default;
    ComponentCache& operator=(ComponentCache&&) = default;

    //------------------------------------------------------------------------
    //! @brief キャッシュからコンポーネントを取得、または取得して更新
    //! @tparam T コンポーネント型
    //! @param world Worldへのポインタ
    //! @param actor 対象Actor
    //! @return コンポーネントへのポインタ（存在しない場合はnullptr）
    //------------------------------------------------------------------------
    template<typename T>
    T* GetOrFetch(World* world, Actor actor);

    //------------------------------------------------------------------------
    //! @brief キャッシュをクリア
    //------------------------------------------------------------------------
    void Clear() noexcept {
        for (auto& entry : fastPath_) {
            entry.Clear();
        }
        overflow_.clear();
    }

    //------------------------------------------------------------------------
    //! @brief 特定の型のキャッシュを無効化
    //! @tparam T コンポーネント型
    //------------------------------------------------------------------------
    template<typename T>
    void Invalidate() noexcept {
        size_t slot = GetTypeSlot<T>();
        if (slot < kFastPathSize) {
            fastPath_[slot].Clear();
        } else {
            auto it = overflow_.find(std::type_index(typeid(T)));
            if (it != overflow_.end()) {
                it->second.Clear();
            }
        }
    }

private:
    //! キャッシュエントリ
    struct CacheEntry {
        void* ptr = nullptr;           //!< キャッシュされたポインタ
        uint32_t frame = UINT32_MAX;   //!< キャッシュしたフレーム
        uint32_t padding = 0;          //!< アラインメント用パディング

        void Clear() noexcept {
            ptr = nullptr;
            frame = UINT32_MAX;
        }

        [[nodiscard]] bool IsValid(uint32_t currentFrame) const noexcept {
            return frame == currentFrame;
        }
    };

    //------------------------------------------------------------------------
    //! @brief 型ごとの固定スロットを取得
    //!
    //! コンパイル時に各型に固定のスロット番号を割り当てる。
    //! スロット番号は登録順で増加し、kFastPathSizeを超えたらオーバーフロー。
    //------------------------------------------------------------------------
    template<typename T>
    static size_t GetTypeSlot() noexcept {
        static const size_t slot = nextSlot_++;
        return slot;
    }

    //! 次のスロット番号（静的カウンター）
    static inline size_t nextSlot_ = 0;

    //! 高速パス（最初の8型）
    std::array<CacheEntry, kFastPathSize> fastPath_{};

    //! オーバーフロー（9型目以降）
    std::unordered_map<std::type_index, CacheEntry> overflow_;
};

} // namespace ECS

// 実装はworld.hのインクルード後に必要（循環依存回避）
// component_cache_impl.h で定義
