//----------------------------------------------------------------------------
//! @file   collision2d_system.h
//! @brief  ECS Collision2DSystem - 2D衝突判定システム
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/collision/collider2d_data.h"
#include "engine/ecs/collision/collision_event_queue.h"
#include "engine/ecs/collision/spatial_hash_2d.h"
#include <cmath>

namespace ECS {

//============================================================================
//! @brief 2D衝突判定システム（クエリシステム）
//!
//! 入力: LocalTransform, Collider2DData（読み取り専用）
//! 出力: EventQueue2D
//!
//! 処理フロー:
//! 1. LocalTransformからCollider2Dのposを同期
//! 2. SpatialHashGridに全コライダーを登録
//! 3. Broad-phase: 同一セル内のペア抽出
//! 4. Narrow-phase: AABB判定
//! 5. CollisionEventQueueにイベント追加
//!
//! @note 優先度10（TransformSystemの後）
//============================================================================
class Collision2DSystem final : public ISystem {
public:
    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param cellSize 空間ハッシュのセルサイズ
    //------------------------------------------------------------------------
    explicit Collision2DSystem(float cellSize = 128.0f)
        : spatialHash_(cellSize) {}

    //------------------------------------------------------------------------
    //! @brief システム実行
    //------------------------------------------------------------------------
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        eventQueue_.BeginFrame();
        spatialHash_.Clear();

        // 1. Position同期 + SpatialHash登録
        world.ForEach<LocalTransform, Collider2DData>(
            [this](Actor actor, const LocalTransform& transform, Collider2DData& c) {
                if (!c.IsEnabled()) return;

                // LocalTransformからCollider位置を更新
                c.posX = transform.position.x + c.offsetX;
                c.posY = transform.position.y + c.offsetY;

                // SpatialHashに登録
                spatialHash_.Insert(actor, c.posX, c.posY, c.halfW, c.halfH);
            });

        // 2. Broad-phase + Narrow-phase
        spatialHash_.QueryAllPairs([this, &world](Actor a, Actor b) {
            auto* cA = world.GetComponent<Collider2DData>(a);
            auto* cB = world.GetComponent<Collider2DData>(b);
            if (!cA || !cB) return;

            // レイヤーマスクチェック
            if ((cA->layer & cB->mask) == 0 || (cB->layer & cA->mask) == 0) return;

            // AABB判定
            if (AABBIntersects(*cA, *cB)) {
                Collision::Event2D event;
                event.actorA = a;
                event.actorB = b;
                event.layerA = cA->layer;
                event.layerB = cB->layer;
                // 接触点・法線・侵入深度の計算
                ComputeContactInfo(*cA, *cB, event);
                eventQueue_.Push(event);
            }
        });

        eventQueue_.EndFrame();
    }

    int Priority() const override { return 10; }
    const char* Name() const override { return "Collision2DSystem"; }

    //------------------------------------------------------------------------
    //! @brief イベントキューへのアクセス
    //------------------------------------------------------------------------
    [[nodiscard]] const Collision::EventQueue2D& GetEventQueue() const noexcept {
        return eventQueue_;
    }

    //------------------------------------------------------------------------
    //! @brief セルサイズを設定
    //------------------------------------------------------------------------
    void SetCellSize(float size) noexcept {
        spatialHash_.SetCellSize(size);
    }

private:
    //------------------------------------------------------------------------
    //! @brief AABB交差判定
    //------------------------------------------------------------------------
    [[nodiscard]] static bool AABBIntersects(const Collider2DData& a, const Collider2DData& b) noexcept {
        return (a.posX - a.halfW < b.posX + b.halfW) &&
               (a.posX + a.halfW > b.posX - b.halfW) &&
               (a.posY - a.halfH < b.posY + b.halfH) &&
               (a.posY + a.halfH > b.posY - b.halfH);
    }

    //------------------------------------------------------------------------
    //! @brief 接触情報を計算
    //------------------------------------------------------------------------
    static void ComputeContactInfo(const Collider2DData& a, const Collider2DData& b,
                                   Collision::Event2D& event) noexcept {
        // 接触点（2つのAABB中心の中点）
        event.contactX = (a.posX + b.posX) * 0.5f;
        event.contactY = (a.posY + b.posY) * 0.5f;

        // 侵入深度と法線（最小分離軸）
        float overlapX = (a.halfW + b.halfW) - std::abs(a.posX - b.posX);
        float overlapY = (a.halfH + b.halfH) - std::abs(a.posY - b.posY);

        if (overlapX < overlapY) {
            event.normalX = (a.posX < b.posX) ? -1.0f : 1.0f;
            event.normalY = 0.0f;
            event.penetration = overlapX;
        } else {
            event.normalX = 0.0f;
            event.normalY = (a.posY < b.posY) ? -1.0f : 1.0f;
            event.penetration = overlapY;
        }
    }

    Collision::SpatialHash2D spatialHash_;
    Collision::EventQueue2D eventQueue_;
};

} // namespace ECS
