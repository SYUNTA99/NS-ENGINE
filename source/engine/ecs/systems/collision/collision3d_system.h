//----------------------------------------------------------------------------
//! @file   collision3d_system.h
//! @brief  ECS Collision3DSystem - 3D衝突判定システム
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/collision/collider3d_data.h"
#include "engine/ecs/collision/collision_event_queue.h"
#include "engine/ecs/collision/spatial_grid_3d.h"
#include <cmath>

namespace ECS {

//============================================================================
//! @brief 3D衝突判定システム（クエリシステム）
//!
//! 入力: LocalToWorldData, Collider3DData（読み取り専用）
//! 出力: EventQueue3D
//!
//! 処理フロー:
//! 1. LocalToWorldDataからCollider3DのAABB境界を同期
//! 2. SpatialGrid3Dに全コライダーを登録
//! 3. Broad-phase: 同一セル内のペア抽出
//! 4. Narrow-phase: 形状に応じた詳細判定
//! 5. CollisionEventQueueにイベント追加
//!
//! @note 優先度11（Collision2DSystemの後）
//============================================================================
class Collision3DSystem final : public ISystem {
public:
    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param cellSize 空間グリッドのセルサイズ
    //------------------------------------------------------------------------
    explicit Collision3DSystem(float cellSize = 10.0f)
        : spatialGrid_(cellSize) {}

    //------------------------------------------------------------------------
    //! @brief システム実行
    //------------------------------------------------------------------------
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        eventQueue_.BeginFrame();
        spatialGrid_.Clear();

        // 1. LocalToWorld同期 + SpatialGrid登録
        world.ForEach<LocalToWorldData, Collider3DData>(
            [this](Actor actor, const LocalToWorldData& ltw, Collider3DData& c) {
                if (!c.IsEnabled()) return;

                // LocalToWorldDataからAABB境界を更新
                Vector3 worldPos = ltw.GetPosition();
                c.UpdateBounds(worldPos);

                // SpatialGridに登録
                spatialGrid_.Insert(actor,
                    c.minX, c.minY, c.minZ,
                    c.maxX, c.maxY, c.maxZ);
            });

        // 2. Broad-phase + Narrow-phase
        spatialGrid_.QueryAllPairs([this, &world](Actor a, Actor b) {
            auto* cA = world.GetComponent<Collider3DData>(a);
            auto* cB = world.GetComponent<Collider3DData>(b);
            if (!cA || !cB) return;

            // レイヤーマスクチェック
            if ((cA->layer & cB->mask) == 0 || (cB->layer & cA->mask) == 0) return;

            // 詳細判定
            Collision::Event3D event;
            if (TestCollision(*cA, *cB, event)) {
                event.actorA = a;
                event.actorB = b;
                event.layerA = cA->layer;
                event.layerB = cB->layer;
                eventQueue_.Push(event);
            }
        });

        eventQueue_.EndFrame();
    }

    int Priority() const override { return 11; }
    const char* Name() const override { return "Collision3DSystem"; }

    //------------------------------------------------------------------------
    //! @brief イベントキューへのアクセス
    //------------------------------------------------------------------------
    [[nodiscard]] const Collision::EventQueue3D& GetEventQueue() const noexcept {
        return eventQueue_;
    }

    //------------------------------------------------------------------------
    //! @brief セルサイズを設定
    //------------------------------------------------------------------------
    void SetCellSize(float size) noexcept {
        spatialGrid_.SetCellSize(size);
    }

private:
    //------------------------------------------------------------------------
    //! @brief 衝突判定（形状に応じたディスパッチ）
    //------------------------------------------------------------------------
    [[nodiscard]] static bool TestCollision(const Collider3DData& a,
                                             const Collider3DData& b,
                                             Collision::Event3D& event) noexcept {
        // まずAABBで除外判定（ブロードフェーズですでにAABBは重なっているはず）
        if (!AABBIntersects(a, b)) return false;

        // 形状に応じた詳細判定
        if (a.shapeType == Collider3DShape::Sphere &&
            b.shapeType == Collider3DShape::Sphere) {
            return SphereSphere(a, b, event);
        }
        else if (a.shapeType == Collider3DShape::AABB &&
                 b.shapeType == Collider3DShape::AABB) {
            return AABBAABB(a, b, event);
        }
        else if (a.shapeType == Collider3DShape::Sphere &&
                 b.shapeType == Collider3DShape::AABB) {
            return SphereAABB(a, b, event);
        }
        else if (a.shapeType == Collider3DShape::AABB &&
                 b.shapeType == Collider3DShape::Sphere) {
            bool result = SphereAABB(b, a, event);
            if (result) {
                // 法線を反転
                event.normalX = -event.normalX;
                event.normalY = -event.normalY;
                event.normalZ = -event.normalZ;
            }
            return result;
        }
        else if (a.shapeType == Collider3DShape::Capsule ||
                 b.shapeType == Collider3DShape::Capsule) {
            // カプセル判定はAABB近似で代用
            return AABBAABB(a, b, event);
        }

        // フォールバック: AABB判定
        return AABBAABB(a, b, event);
    }

    //------------------------------------------------------------------------
    //! @brief AABB交差判定
    //------------------------------------------------------------------------
    [[nodiscard]] static bool AABBIntersects(const Collider3DData& a,
                                              const Collider3DData& b) noexcept {
        return (a.minX <= b.maxX && a.maxX >= b.minX) &&
               (a.minY <= b.maxY && a.maxY >= b.minY) &&
               (a.minZ <= b.maxZ && a.maxZ >= b.minZ);
    }

    //------------------------------------------------------------------------
    //! @brief AABB vs AABB詳細判定
    //------------------------------------------------------------------------
    [[nodiscard]] static bool AABBAABB(const Collider3DData& a,
                                        const Collider3DData& b,
                                        Collision::Event3D& event) noexcept {
        // 接触点（2つのAABB中心の中点）
        Vector3 centerA = a.GetCenter();
        Vector3 centerB = b.GetCenter();
        event.contactX = (centerA.x + centerB.x) * 0.5f;
        event.contactY = (centerA.y + centerB.y) * 0.5f;
        event.contactZ = (centerA.z + centerB.z) * 0.5f;

        // 侵入深度と法線（最小分離軸）
        float halfExtentAx = (a.maxX - a.minX) * 0.5f;
        float halfExtentAy = (a.maxY - a.minY) * 0.5f;
        float halfExtentAz = (a.maxZ - a.minZ) * 0.5f;
        float halfExtentBx = (b.maxX - b.minX) * 0.5f;
        float halfExtentBy = (b.maxY - b.minY) * 0.5f;
        float halfExtentBz = (b.maxZ - b.minZ) * 0.5f;

        float overlapX = (halfExtentAx + halfExtentBx) - std::abs(centerA.x - centerB.x);
        float overlapY = (halfExtentAy + halfExtentBy) - std::abs(centerA.y - centerB.y);
        float overlapZ = (halfExtentAz + halfExtentBz) - std::abs(centerA.z - centerB.z);

        if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0) return false;

        // 最小分離軸を選択
        if (overlapX < overlapY && overlapX < overlapZ) {
            event.normalX = (centerA.x < centerB.x) ? -1.0f : 1.0f;
            event.normalY = 0.0f;
            event.normalZ = 0.0f;
            event.penetration = overlapX;
        } else if (overlapY < overlapZ) {
            event.normalX = 0.0f;
            event.normalY = (centerA.y < centerB.y) ? -1.0f : 1.0f;
            event.normalZ = 0.0f;
            event.penetration = overlapY;
        } else {
            event.normalX = 0.0f;
            event.normalY = 0.0f;
            event.normalZ = (centerA.z < centerB.z) ? -1.0f : 1.0f;
            event.penetration = overlapZ;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! @brief Sphere vs Sphere判定
    //------------------------------------------------------------------------
    [[nodiscard]] static bool SphereSphere(const Collider3DData& a,
                                            const Collider3DData& b,
                                            Collision::Event3D& event) noexcept {
        Vector3 centerA = a.GetCenter();
        Vector3 centerB = b.GetCenter();
        float radiusA = a.shape.sphere.radius;
        float radiusB = b.shape.sphere.radius;

        float dx = centerB.x - centerA.x;
        float dy = centerB.y - centerA.y;
        float dz = centerB.z - centerA.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        float radiusSum = radiusA + radiusB;

        if (distSq > radiusSum * radiusSum) return false;

        float dist = std::sqrt(distSq);
        if (dist < 0.0001f) {
            // 中心が同じ位置の場合
            event.normalX = 0.0f;
            event.normalY = 1.0f;
            event.normalZ = 0.0f;
            event.penetration = radiusSum;
        } else {
            float invDist = 1.0f / dist;
            event.normalX = dx * invDist;
            event.normalY = dy * invDist;
            event.normalZ = dz * invDist;
            event.penetration = radiusSum - dist;
        }

        // 接触点（2つの球の表面間の中点）
        float contactDist = radiusA - event.penetration * 0.5f;
        event.contactX = centerA.x + event.normalX * contactDist;
        event.contactY = centerA.y + event.normalY * contactDist;
        event.contactZ = centerA.z + event.normalZ * contactDist;

        return true;
    }

    //------------------------------------------------------------------------
    //! @brief Sphere vs AABB判定
    //------------------------------------------------------------------------
    [[nodiscard]] static bool SphereAABB(const Collider3DData& sphere,
                                          const Collider3DData& aabb,
                                          Collision::Event3D& event) noexcept {
        Vector3 sphereCenter = sphere.GetCenter();
        float radius = sphere.shape.sphere.radius;

        // AABBへの最近接点を計算
        float closestX = std::clamp(sphereCenter.x, aabb.minX, aabb.maxX);
        float closestY = std::clamp(sphereCenter.y, aabb.minY, aabb.maxY);
        float closestZ = std::clamp(sphereCenter.z, aabb.minZ, aabb.maxZ);

        // 距離を計算
        float dx = sphereCenter.x - closestX;
        float dy = sphereCenter.y - closestY;
        float dz = sphereCenter.z - closestZ;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq > radius * radius) return false;

        float dist = std::sqrt(distSq);
        if (dist < 0.0001f) {
            // 球の中心がAABB内部にある
            Vector3 aabbCenter = aabb.GetCenter();
            dx = sphereCenter.x - aabbCenter.x;
            dy = sphereCenter.y - aabbCenter.y;
            dz = sphereCenter.z - aabbCenter.z;

            float halfExtentX = (aabb.maxX - aabb.minX) * 0.5f;
            float halfExtentY = (aabb.maxY - aabb.minY) * 0.5f;
            float halfExtentZ = (aabb.maxZ - aabb.minZ) * 0.5f;

            float overlapX = halfExtentX - std::abs(dx);
            float overlapY = halfExtentY - std::abs(dy);
            float overlapZ = halfExtentZ - std::abs(dz);

            if (overlapX < overlapY && overlapX < overlapZ) {
                event.normalX = (dx > 0) ? 1.0f : -1.0f;
                event.normalY = 0.0f;
                event.normalZ = 0.0f;
                event.penetration = overlapX + radius;
            } else if (overlapY < overlapZ) {
                event.normalX = 0.0f;
                event.normalY = (dy > 0) ? 1.0f : -1.0f;
                event.normalZ = 0.0f;
                event.penetration = overlapY + radius;
            } else {
                event.normalX = 0.0f;
                event.normalY = 0.0f;
                event.normalZ = (dz > 0) ? 1.0f : -1.0f;
                event.penetration = overlapZ + radius;
            }
        } else {
            float invDist = 1.0f / dist;
            event.normalX = dx * invDist;
            event.normalY = dy * invDist;
            event.normalZ = dz * invDist;
            event.penetration = radius - dist;
        }

        event.contactX = closestX;
        event.contactY = closestY;
        event.contactZ = closestZ;

        return true;
    }

    Collision::SpatialGrid3D spatialGrid_;
    Collision::EventQueue3D eventQueue_;
};

} // namespace ECS
