//----------------------------------------------------------------------------
//! @file   collision_manager.h
//! @brief  衝突判定マネージャー（Unity風設計）
//!
//! @note Collider2Dがデータを保持し、このマネージャーは
//!       コライダーへの参照を管理して衝突検出を行う。
//!
//! @note スレッドセーフ性:
//!       【警告】このクラスはスレッドセーフではありません。
//!       全メソッドはメインスレッドからのみ呼び出し可能。
//----------------------------------------------------------------------------
#pragma once

#include "common/utility/non_copyable.h"
#include "engine/math/math_types.h"
#include "engine/core/singleton_registry.h"
#include "engine/component/collider2d.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <cassert>

//============================================================================
// 定数定義
//============================================================================
namespace CollisionConstants {
    static constexpr uint8_t kDefaultLayer = 0x01;  //!< デフォルトレイヤー
    static constexpr uint8_t kDefaultMask = 0xFF;   //!< デフォルトマスク
    static constexpr int kDefaultCellSize = 256;    //!< デフォルトセルサイズ
}

//============================================================================
//! @brief レイキャストヒット情報
//============================================================================
struct RaycastHit {
    Collider2D* collider = nullptr;  //!< ヒットしたコライダー
    float distance = 0.0f;           //!< 始点からの距離
    Vector2 point;                   //!< ヒット座標
};

//============================================================================
//! @brief 衝突判定マネージャー
//!
//! Collider2Dへの参照を管理し、空間ハッシュグリッドで衝突検出を行う。
//============================================================================
class CollisionManager final : private NonCopyableNonMovable {
public:
    //! @brief シングルトンインスタンス取得
    static CollisionManager& Get()
    {
        assert(instance_ && "CollisionManager::Create() must be called first");
        return *instance_;
    }

    //! @brief インスタンス生成
    static void Create()
    {
        if (!instance_) {
            instance_ = std::unique_ptr<CollisionManager>(new CollisionManager());
            SINGLETON_REGISTER(CollisionManager, SingletonId::None);
        }
    }

    //! @brief インスタンス破棄
    static void Destroy()
    {
        if (instance_) {
            SINGLETON_UNREGISTER(CollisionManager);
            instance_.reset();
        }
    }

    ~CollisionManager() = default;

    //------------------------------------------------------------------------
    // 初期化・終了
    //------------------------------------------------------------------------

    void Initialize(int cellSize = CollisionConstants::kDefaultCellSize);
    void Shutdown();

    //------------------------------------------------------------------------
    // コライダー登録（Collider2Dから呼ばれる）
    //------------------------------------------------------------------------

    //! @brief コライダーを登録
    void Register(Collider2D* collider);

    //! @brief コライダーを解除
    void Unregister(Collider2D* collider);

    //! @brief 全コライダーをクリア
    void Clear();

    //------------------------------------------------------------------------
    // 更新
    //------------------------------------------------------------------------

    //! @brief 衝突判定を実行（固定タイムステップ）
    void Update(float deltaTime);

    //! @brief 固定タイムステップの間隔を取得
    [[nodiscard]] static constexpr float GetFixedDeltaTime() noexcept { return kFixedDeltaTime; }

    //------------------------------------------------------------------------
    // 設定・統計
    //------------------------------------------------------------------------

    void SetCellSize(int size) noexcept {
        cellSize_ = size > 0 ? size : CollisionConstants::kDefaultCellSize;
    }
    [[nodiscard]] int GetCellSize() const noexcept { return cellSize_; }
    [[nodiscard]] size_t GetColliderCount() const noexcept { return colliders_.size(); }

    //------------------------------------------------------------------------
    // クエリ
    //------------------------------------------------------------------------

    //! @brief AABB範囲内のコライダーを検索
    void QueryAABB(const AABB& aabb, std::vector<Collider2D*>& results,
                   uint8_t layerMask = CollisionConstants::kDefaultMask);

    //! @brief 点と交差するコライダーを検索
    void QueryPoint(const Vector2& point, std::vector<Collider2D*>& results,
                    uint8_t layerMask = CollisionConstants::kDefaultMask);

    //! @brief 線分と交差するコライダーを検索
    void QueryLineSegment(const Vector2& start, const Vector2& end,
                          std::vector<Collider2D*>& results,
                          uint8_t layerMask = CollisionConstants::kDefaultMask);

    //! @brief レイキャストで最初にヒットしたコライダーを取得
    [[nodiscard]] std::optional<RaycastHit> RaycastFirst(
        const Vector2& start, const Vector2& end,
        uint8_t layerMask = CollisionConstants::kDefaultMask);

private:
    CollisionManager() = default;

    static inline std::unique_ptr<CollisionManager> instance_ = nullptr;

    //! @brief 固定タイムステップの衝突判定
    void FixedUpdate();

    //------------------------------------------------------------------------
    // 空間ハッシュグリッド
    //------------------------------------------------------------------------

    struct Cell {
        int x, y;
        bool operator==(const Cell& other) const noexcept {
            return x == other.x && y == other.y;
        }
    };

    struct CellHash {
        size_t operator()(const Cell& c) const noexcept {
            auto h1 = std::hash<int>{}(c.x);
            auto h2 = std::hash<int>{}(c.y);
            return h1 ^ (h2 * 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };

    [[nodiscard]] Cell ToCell(float x, float y) const noexcept;
    void RebuildGrid();

    //------------------------------------------------------------------------
    // ペア管理
    //------------------------------------------------------------------------

    struct ColliderPair {
        Collider2D* a;
        Collider2D* b;
        bool operator==(const ColliderPair& other) const noexcept {
            return (a == other.a && b == other.b) || (a == other.b && b == other.a);
        }
    };

    struct ColliderPairHash {
        size_t operator()(const ColliderPair& p) const noexcept {
            auto h1 = std::hash<void*>{}(p.a);
            auto h2 = std::hash<void*>{}(p.b);
            return h1 ^ h2;
        }
    };

    //------------------------------------------------------------------------
    // データ
    //------------------------------------------------------------------------

    // 登録済みコライダー
    std::vector<Collider2D*> colliders_;

    // 空間ハッシュグリッド
    int cellSize_ = CollisionConstants::kDefaultCellSize;
    std::unordered_map<Cell, std::vector<Collider2D*>, CellHash> grid_;

    // 衝突ペア（前フレーム / 現フレーム）
    std::unordered_set<ColliderPair, ColliderPairHash> previousPairs_;
    std::unordered_set<ColliderPair, ColliderPairHash> currentPairs_;

    // 固定タイムステップ
    static constexpr float kFixedDeltaTime = 1.0f / 60.0f;
    float accumulator_ = 0.0f;
};
