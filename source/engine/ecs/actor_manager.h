//----------------------------------------------------------------------------
//! @file   actor_manager.h
//! @brief  ECS ActorManager - アクターの生成・破棄・管理
//----------------------------------------------------------------------------
#pragma once

#include "actor.h"
#include <vector>
#include <cassert>

namespace ECS {

//============================================================================
//! @brief アクターマネージャー
//!
//! アクターの生成、破棄、生存確認を管理する。
//! フリーリストを使用してアクターIDを再利用し、
//! 世代番号で古い参照を検出する。
//============================================================================
class ActorManager {
public:
    ActorManager() = default;
    ~ActorManager() = default;

    // コピー禁止、ムーブ許可
    ActorManager(const ActorManager&) = delete;
    ActorManager& operator=(const ActorManager&) = delete;
    ActorManager(ActorManager&&) = default;
    ActorManager& operator=(ActorManager&&) = default;

    //------------------------------------------------------------------------
    //! @brief 新しいアクターを生成
    //! @return 生成されたアクター
    //------------------------------------------------------------------------
    [[nodiscard]] Actor Create() {
        uint32_t index;

        if (!freeList_.empty()) {
            // フリーリストから再利用
            index = freeList_.back();
            freeList_.pop_back();
            alive_[index] = true;
        } else {
            // 新規スロット割り当て
            index = static_cast<uint32_t>(generations_.size());
            generations_.push_back(0);
            alive_.push_back(true);
        }

        ++aliveCount_;
        return Actor(index, generations_[index]);
    }

    //------------------------------------------------------------------------
    //! @brief アクターを破棄
    //! @param a 破棄するアクター
    //------------------------------------------------------------------------
    void Destroy(Actor a) {
        if (!IsAlive(a)) {
            return;
        }

        uint32_t index = a.Index();

        // 世代番号をインクリメント（古い参照を無効化）
        generations_[index] = (generations_[index] + 1) & Actor::kGenerationMask;

        // 生存フラグをオフ
        alive_[index] = false;

        // フリーリストに追加
        freeList_.push_back(index);
        --aliveCount_;
    }

    //------------------------------------------------------------------------
    //! @brief アクターが生存しているか確認
    //! @param a 確認するアクター
    //! @return 生存している場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsAlive(Actor a) const {
        if (!a.IsValid()) {
            return false;
        }

        uint32_t index = a.Index();
        if (index >= generations_.size()) {
            return false;
        }

        // 世代が一致し、かつ生存フラグがオンの場合のみ生存
        return alive_[index] && generations_[index] == a.Generation();
    }

    //------------------------------------------------------------------------
    //! @brief 生存しているアクター数を取得
    //! @return 生存アクター数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Count() const noexcept {
        return aliveCount_;
    }

    //------------------------------------------------------------------------
    //! @brief 全アクターをクリア
    //------------------------------------------------------------------------
    void Clear() {
        generations_.clear();
        alive_.clear();
        freeList_.clear();
        aliveCount_ = 0;
    }

    //------------------------------------------------------------------------
    //! @brief 生存している全アクターに対して処理を実行
    //! @tparam Func 処理関数の型
    //! @param func 各アクターに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename Func>
    void ForEach(Func&& func) const {
        for (uint32_t i = 0; i < generations_.size(); ++i) {
            if (alive_[i]) {
                Actor a(i, generations_[i]);
                func(a);
            }
        }
    }

private:
    std::vector<uint16_t> generations_;  //!< 各インデックスの現在の世代番号
    std::vector<bool> alive_;            //!< 各インデックスの生存フラグ
    std::vector<uint32_t> freeList_;     //!< 再利用可能なインデックス
    size_t aliveCount_ = 0;              //!< 生存アクター数
};

} // namespace ECS
