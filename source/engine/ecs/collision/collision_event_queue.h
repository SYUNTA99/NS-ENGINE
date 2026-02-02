//----------------------------------------------------------------------------
//! @file   collision_event_queue.h
//! @brief  衝突イベントキュー（フレーム単位）
//----------------------------------------------------------------------------
#pragma once

#include "collision_event.h"
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <algorithm>

namespace Collision {

//============================================================================
//! @brief 衝突ペアキー（重複検出用）
//============================================================================
struct PairKey {
    uint64_t key;

    PairKey(ECS::Actor a, ECS::Actor b) noexcept {
        // 順序を正規化してユニークキーを生成
        uint32_t idA = a.GetId(), idB = b.GetId();
        if (idA > idB) std::swap(idA, idB);
        key = (static_cast<uint64_t>(idA) << 32) | idB;
    }

    bool operator==(const PairKey& other) const noexcept {
        return key == other.key;
    }
};

} // namespace Collision

namespace std {
template<>
struct hash<Collision::PairKey> {
    size_t operator()(const Collision::PairKey& k) const noexcept {
        return static_cast<size_t>(k.key);
    }
};
}

namespace Collision {

//============================================================================
//! @brief 2D衝突イベントキュー
//!
//! フレーム毎にクリアされる一時的なイベント格納。
//! Enter/Stay/Exit判定のために前フレームの衝突ペアを保持。
//============================================================================
class EventQueue2D {
public:
    //------------------------------------------------------------------------
    //! @brief フレーム開始時にクリア
    //------------------------------------------------------------------------
    void BeginFrame() noexcept {
        events_.clear();
        // 初回のみreserve（リアロケーション防止）
        if (events_.capacity() < kDefaultEventCapacity) {
            events_.reserve(kDefaultEventCapacity);
        }
        currentPairs_.clear();
    }

private:
    //! デフォルトのイベント容量（典型的な衝突ペア数）
    static constexpr size_t kDefaultEventCapacity = 256;

public:

    //------------------------------------------------------------------------
    //! @brief 衝突イベントを追加
    //------------------------------------------------------------------------
    void Push(const Event2D& event) {
        currentPairs_.insert(PairKey(event.actorA, event.actorB));
        events_.push_back(event);
    }

    //------------------------------------------------------------------------
    //! @brief フレーム終了時にEnter/Stay/Exit判定を確定
    //------------------------------------------------------------------------
    void EndFrame() {
        // 前フレームに存在し、今フレームに存在しないペア → Exit
        for (const auto& prevKey : previousPairs_) {
            if (currentPairs_.find(prevKey) == currentPairs_.end()) {
                // Exit イベントを生成
                Event2D exitEvent;
                exitEvent.actorA = ECS::Actor(static_cast<uint32_t>(prevKey.key >> 32));
                exitEvent.actorB = ECS::Actor(static_cast<uint32_t>(prevKey.key & 0xFFFFFFFF));
                exitEvent.type = EventType::Exit;
                events_.push_back(exitEvent);
            }
        }

        // Enter/Stayの判定（currentに存在するものを検査）
        for (auto& event : events_) {
            if (event.type == EventType::Exit) continue;
            PairKey key(event.actorA, event.actorB);
            if (previousPairs_.find(key) != previousPairs_.end()) {
                event.type = EventType::Stay;
            } else {
                event.type = EventType::Enter;
            }
        }

        // ペアセットをスワップ
        previousPairs_ = std::move(currentPairs_);
        currentPairs_.clear();
    }

    //------------------------------------------------------------------------
    //! @brief イベント配列への読み取りアクセス
    //------------------------------------------------------------------------
    [[nodiscard]] const std::vector<Event2D>& GetEvents() const noexcept {
        return events_;
    }

    //------------------------------------------------------------------------
    //! @brief イベント数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Count() const noexcept { return events_.size(); }

    //------------------------------------------------------------------------
    //! @brief 全クリア
    //------------------------------------------------------------------------
    void Clear() noexcept {
        events_.clear();
        currentPairs_.clear();
        previousPairs_.clear();
    }

private:
    std::vector<Event2D> events_;
    std::unordered_set<PairKey> currentPairs_;
    std::unordered_set<PairKey> previousPairs_;
};

//============================================================================
//! @brief 3D衝突イベントキュー
//!
//! 2Dキューと同様のパターン。
//============================================================================
class EventQueue3D {
public:
    void BeginFrame() noexcept {
        events_.clear();
        // 初回のみreserve（リアロケーション防止）
        if (events_.capacity() < kDefaultEventCapacity) {
            events_.reserve(kDefaultEventCapacity);
        }
        currentPairs_.clear();
    }

private:
    //! デフォルトのイベント容量
    static constexpr size_t kDefaultEventCapacity = 256;

public:

    void Push(const Event3D& event) {
        currentPairs_.insert(PairKey(event.actorA, event.actorB));
        events_.push_back(event);
    }

    void EndFrame() {
        // 前フレームに存在し、今フレームに存在しないペア → Exit
        for (const auto& prevKey : previousPairs_) {
            if (currentPairs_.find(prevKey) == currentPairs_.end()) {
                Event3D exitEvent;
                exitEvent.actorA = ECS::Actor(static_cast<uint32_t>(prevKey.key >> 32));
                exitEvent.actorB = ECS::Actor(static_cast<uint32_t>(prevKey.key & 0xFFFFFFFF));
                exitEvent.type = EventType::Exit;
                events_.push_back(exitEvent);
            }
        }

        // Enter/Stayの判定
        for (auto& event : events_) {
            if (event.type == EventType::Exit) continue;
            PairKey key(event.actorA, event.actorB);
            if (previousPairs_.find(key) != previousPairs_.end()) {
                event.type = EventType::Stay;
            } else {
                event.type = EventType::Enter;
            }
        }

        previousPairs_ = std::move(currentPairs_);
        currentPairs_.clear();
    }

    [[nodiscard]] const std::vector<Event3D>& GetEvents() const noexcept {
        return events_;
    }

    [[nodiscard]] size_t Count() const noexcept { return events_.size(); }

    void Clear() noexcept {
        events_.clear();
        currentPairs_.clear();
        previousPairs_.clear();
    }

private:
    std::vector<Event3D> events_;
    std::unordered_set<PairKey> currentPairs_;
    std::unordered_set<PairKey> previousPairs_;
};

} // namespace Collision
