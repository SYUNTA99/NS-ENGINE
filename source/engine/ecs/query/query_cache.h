//----------------------------------------------------------------------------
//! @file   query_cache.h
//! @brief  ECS QueryCache - Queryマッチング結果のキャッシュ
//----------------------------------------------------------------------------
#pragma once


#include "../archetype.h"
#include <unordered_map>
#include <vector>
#include <typeindex>

namespace ECS {

// 前方宣言
class ArchetypeStorage;

//============================================================================
//! @brief QueryCache
//!
//! Query<Ts...>のマッチング結果をキャッシュする。
//! Archetype追加時にキャッシュを無効化し、次回Queryで再構築。
//!
//! 性能向上:
//! - ForEachMatching: O(Archetype数) → O(マッチArchetype数)
//! - 10万Entity + 100 Archetype規模で効果的
//!
//! @note メインスレッドからのみ使用すること
//============================================================================
class QueryCache {
public:
    //! キャッシュエントリ
    struct CacheEntry {
        std::vector<Archetype*> archetypes;
        uint32_t version = 0;  //!< 構築時のバージョン
    };

    QueryCache() = default;
    ~QueryCache() = default;

    // コピー禁止
    QueryCache(const QueryCache&) = delete;
    QueryCache& operator=(const QueryCache&) = delete;

    //------------------------------------------------------------------------
    //! @brief キャッシュを無効化
    //!
    //! Archetype追加/削除時に呼び出す。
    //! 次回のGetOrBuild呼び出しでキャッシュが再構築される。
    //------------------------------------------------------------------------
    void Invalidate() noexcept {
        ++version_;
    }

    //------------------------------------------------------------------------
    //! @brief 現在のバージョンを取得
    //------------------------------------------------------------------------
    [[nodiscard]] uint32_t GetVersion() const noexcept {
        return version_;
    }

    //------------------------------------------------------------------------
    //! @brief キャッシュキーを計算
    //! @tparam Ts コンポーネント型群
    //! @return ハッシュ値（キャッシュキー）
    //------------------------------------------------------------------------
    template<typename... Ts>
    [[nodiscard]] static size_t CalculateKey() {
        // 型のtype_indexを使ってFNV-1aハッシュを計算
        size_t hash = 14695981039346656037ull;
        ((hash ^= std::type_index(typeid(Ts)).hash_code(),
          hash *= 1099511628211ull), ...);
        return hash;
    }

    //------------------------------------------------------------------------
    //! @brief キャッシュエントリを取得（なければnullptr）
    //! @param key キャッシュキー
    //! @return キャッシュエントリへのポインタ（存在しない場合はnullptr）
    //------------------------------------------------------------------------
    [[nodiscard]] CacheEntry* GetEntry(size_t key) noexcept {
        auto it = cache_.find(key);
        if (it != cache_.end() && it->second.version == version_) {
            return &it->second;
        }
        return nullptr;
    }

    //------------------------------------------------------------------------
    //! @brief キャッシュエントリを設定
    //! @param key キャッシュキー
    //! @param archetypes マッチするArchetypeのリスト
    //------------------------------------------------------------------------
    void SetEntry(size_t key, std::vector<Archetype*> archetypes) {
        CacheEntry entry;
        entry.archetypes = std::move(archetypes);
        entry.version = version_;
        cache_[key] = std::move(entry);
    }

    //------------------------------------------------------------------------
    //! @brief キャッシュをクリア
    //------------------------------------------------------------------------
    void Clear() {
        cache_.clear();
        version_ = 0;
    }

    //------------------------------------------------------------------------
    //! @brief キャッシュエントリ数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetEntryCount() const noexcept {
        return cache_.size();
    }

private:
    std::unordered_map<size_t, CacheEntry> cache_;
    uint32_t version_ = 0;  //!< キャッシュバージョン（無効化でインクリメント）
};

} // namespace ECS
