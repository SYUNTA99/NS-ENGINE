//----------------------------------------------------------------------------
//! @file   deferred_queue.h
//! @brief  ECS DeferredQueue - 遅延操作キュー
//----------------------------------------------------------------------------
#pragma once


#include "common/stl/stl_common.h"
#include "common/stl/stl_metaprogramming.h"
#include "actor.h"

namespace ECS {

// 前方宣言
class World;

//============================================================================
//! @brief 遅延操作の種類
//============================================================================
enum class DeferredOpType : uint8_t {
    Create,           //!< Actor作成
    Destroy,          //!< Actor破棄
    AddComponent,     //!< コンポーネント追加
    RemoveComponent   //!< コンポーネント削除
};

//============================================================================
//! @brief 遅延操作
//!
//! フレーム中にキューに積まれ、BeginFrame()で一括実行される。
//! 構造変更（Archetype変更）を伴う操作をフレーム境界に集約することで
//! フレーム内のポインタ安定性を保証する。
//============================================================================
struct DeferredOp {
    DeferredOpType type;                          //!< 操作種別
    Actor actor;                                  //!< 対象Actor
    std::type_index componentType;                //!< コンポーネント型（Add/Remove用）
    std::unique_ptr<std::byte[]> componentData;   //!< コンポーネントデータ（Add用）
    size_t componentSize = 0;                     //!< コンポーネントサイズ
    size_t componentAlignment = 0;                //!< コンポーネントアラインメント
    std::function<void(void*)> destructor;        //!< デストラクタ（Add用、キャンセル時に呼ぶ）
    std::function<void(World&, Actor, void*)> applier;  //!< Add適用関数
    std::function<void(World&, Actor)> remover;   //!< Remove適用関数

    // デフォルトコンストラクタ
    DeferredOp()
        : type(DeferredOpType::Create)
        , actor(Actor::Invalid())
        , componentType(typeid(void))
    {}

    // ムーブのみ許可
    DeferredOp(DeferredOp&&) = default;
    DeferredOp& operator=(DeferredOp&&) = default;
    DeferredOp(const DeferredOp&) = delete;
    DeferredOp& operator=(const DeferredOp&) = delete;

    // デストラクタ - 未実行のAdd操作のデータを破棄
    ~DeferredOp() {
        if (componentData && destructor) {
            destructor(componentData.get());
        }
    }
};

//============================================================================
//! @brief 遅延操作キューのRAIIガード
//!
//! スコープ終了時に自動的にキューをクリアする。
//! 例外が発生してもクリアが保証される。
//!
//! @code
//! {
//!     auto guard = queue.ScopedClear();
//!     for (auto& op : queue.GetQueue()) {
//!         op.Execute();  // 例外が発生しても...
//!     }
//! }  // ...ここでクリアされる
//! @endcode
//============================================================================
class DeferredQueueClearGuard {
public:
    explicit DeferredQueueClearGuard(class DeferredQueue& queue) noexcept
        : queue_(&queue) {}

    ~DeferredQueueClearGuard();  // 実装は DeferredQueue クラス後

    // ムーブのみ許可
    DeferredQueueClearGuard(DeferredQueueClearGuard&& other) noexcept
        : queue_(other.queue_) {
        other.queue_ = nullptr;
    }

    DeferredQueueClearGuard& operator=(DeferredQueueClearGuard&& other) noexcept {
        if (this != &other) {
            queue_ = other.queue_;
            other.queue_ = nullptr;
        }
        return *this;
    }

    // コピー禁止
    DeferredQueueClearGuard(const DeferredQueueClearGuard&) = delete;
    DeferredQueueClearGuard& operator=(const DeferredQueueClearGuard&) = delete;

    //! @brief ガードを解除（クリアしない）
    void Release() noexcept { queue_ = nullptr; }

private:
    DeferredQueue* queue_;
};

//============================================================================
//! @brief 遅延操作キュー
//!
//! 構造変更操作をキューに積み、BeginFrame()で一括実行する。
//! これによりフレーム中はデータ構造が安定し、ポインタが有効であることを保証する。
//!
//! スレッドセーフティ: メインスレッドのみで使用すること
//============================================================================
class DeferredQueue {
public:
    DeferredQueue() = default;
    ~DeferredQueue() = default;

    // コピー禁止、ムーブ許可
    DeferredQueue(const DeferredQueue&) = delete;
    DeferredQueue& operator=(const DeferredQueue&) = delete;
    DeferredQueue(DeferredQueue&&) = default;
    DeferredQueue& operator=(DeferredQueue&&) = default;

    //------------------------------------------------------------------------
    //! @brief Actor作成をキューに追加
    //! @param actor 作成するActor（ID予約済み）
    //------------------------------------------------------------------------
    void PushCreate(Actor actor) {
        DeferredOp op;
        op.type = DeferredOpType::Create;
        op.actor = actor;
        queue_.push_back(std::move(op));
    }

    //------------------------------------------------------------------------
    //! @brief Actor破棄をキューに追加
    //! @param actor 破棄するActor
    //------------------------------------------------------------------------
    void PushDestroy(Actor actor) {
        DeferredOp op;
        op.type = DeferredOpType::Destroy;
        op.actor = actor;
        queue_.push_back(std::move(op));
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネント追加をキューに追加
    //! @tparam T コンポーネント型
    //! @param actor 対象Actor
    //! @param component 追加するコンポーネント
    //! @note 実装はdeferred_queue_impl.hにあり、world.h末尾でインクルードされる
    //------------------------------------------------------------------------
    template<typename T>
    void PushAdd(Actor actor, T&& component);

    //------------------------------------------------------------------------
    //! @brief コンポーネント削除をキューに追加
    //! @tparam T コンポーネント型
    //! @param actor 対象Actor
    //! @note 実装はdeferred_queue_impl.hにあり、world.h末尾でインクルードされる
    //------------------------------------------------------------------------
    template<typename T>
    void PushRemove(Actor actor);

    //------------------------------------------------------------------------
    //! @brief キューが空か確認
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsEmpty() const noexcept {
        return queue_.empty();
    }

    //------------------------------------------------------------------------
    //! @brief キューのサイズを取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Size() const noexcept {
        return queue_.size();
    }

    //------------------------------------------------------------------------
    //! @brief キューをクリア（未実行の操作は破棄される）
    //------------------------------------------------------------------------
    void Clear() {
        queue_.clear();
    }

    //------------------------------------------------------------------------
    //! @brief RAIIガードを取得（スコープ終了時に自動クリア）
    //! @return DeferredQueueClearGuard オブジェクト
    //!
    //! 例外が発生してもキューがクリアされることを保証する。
    //!
    //! @code
    //! {
    //!     auto guard = deferred_.ScopedClear();
    //!     for (auto& op : deferred_.GetQueue()) {
    //!         try {
    //!             op.Execute();
    //!         } catch (...) {
    //!             // ログ出力して継続
    //!         }
    //!     }
    //! }  // 自動クリア
    //! @endcode
    //------------------------------------------------------------------------
    [[nodiscard]] DeferredQueueClearGuard ScopedClear() noexcept {
        return DeferredQueueClearGuard(*this);
    }

    //------------------------------------------------------------------------
    //! @brief キューの内容を取得（実行用）
    //! @return 操作キューへの参照
    //------------------------------------------------------------------------
    [[nodiscard]] std::vector<DeferredOp>& GetQueue() noexcept {
        return queue_;
    }

    [[nodiscard]] const std::vector<DeferredOp>& GetQueue() const noexcept {
        return queue_;
    }

private:
    std::vector<DeferredOp> queue_;
};

//----------------------------------------------------------------------------
// DeferredQueueClearGuard デストラクタ実装（DeferredQueue完全型が必要）
//----------------------------------------------------------------------------
inline DeferredQueueClearGuard::~DeferredQueueClearGuard() {
    if (queue_) {
        queue_->Clear();
    }
}

} // namespace ECS
