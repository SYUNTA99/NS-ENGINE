//----------------------------------------------------------------------------
//! @file   collision_manager.cpp
//! @brief  衝突判定マネージャー実装（Unity風設計）
//----------------------------------------------------------------------------

#include "collision_manager.h"
#include <algorithm>
#include <cmath>

//----------------------------------------------------------------------------
void CollisionManager::Initialize(int cellSize)
{
    cellSize_ = cellSize > 0 ? cellSize : CollisionConstants::kDefaultCellSize;
    Clear();
}

//----------------------------------------------------------------------------
void CollisionManager::Shutdown()
{
    Clear();
}

//----------------------------------------------------------------------------
void CollisionManager::Register(Collider2D* collider)
{
    if (!collider) return;

    // 重複チェック
    auto it = std::find(colliders_.begin(), colliders_.end(), collider);
    if (it != colliders_.end()) return;

    colliders_.push_back(collider);
}

//----------------------------------------------------------------------------
void CollisionManager::Unregister(Collider2D* collider)
{
    if (!collider) return;

    auto it = std::find(colliders_.begin(), colliders_.end(), collider);
    if (it != colliders_.end()) {
        // swap-and-pop で O(1) 削除
        *it = colliders_.back();
        colliders_.pop_back();
    }

    // ペアからも削除
    for (auto pairIt = previousPairs_.begin(); pairIt != previousPairs_.end(); ) {
        if (pairIt->a == collider || pairIt->b == collider) {
            pairIt = previousPairs_.erase(pairIt);
        } else {
            ++pairIt;
        }
    }
    for (auto pairIt = currentPairs_.begin(); pairIt != currentPairs_.end(); ) {
        if (pairIt->a == collider || pairIt->b == collider) {
            pairIt = currentPairs_.erase(pairIt);
        } else {
            ++pairIt;
        }
    }
}

//----------------------------------------------------------------------------
void CollisionManager::Clear()
{
    colliders_.clear();
    grid_.clear();
    previousPairs_.clear();
    currentPairs_.clear();
    accumulator_ = 0.0f;
}

//----------------------------------------------------------------------------
void CollisionManager::Update(float deltaTime)
{
    accumulator_ += deltaTime;

    while (accumulator_ >= kFixedDeltaTime) {
        FixedUpdate();
        accumulator_ -= kFixedDeltaTime;
    }
}

//----------------------------------------------------------------------------
void CollisionManager::FixedUpdate()
{
    // グリッド再構築
    RebuildGrid();

    // ペア入れ替え
    std::swap(previousPairs_, currentPairs_);
    currentPairs_.clear();

    // グリッドセルごとに衝突判定
    for (auto& [cell, cellColliders] : grid_) {
        size_t count = cellColliders.size();
        if (count < 2) continue;

        for (size_t i = 0; i + 1 < count; ++i) {
            for (size_t j = i + 1; j < count; ++j) {
                Collider2D* colA = cellColliders[i];
                Collider2D* colB = cellColliders[j];

                // 有効性チェック
                if (!colA->IsColliderEnabled()) continue;
                if (!colB->IsColliderEnabled()) continue;

                // レイヤーマスクチェック
                bool canCollide = colA->CanCollideWith(colB->GetLayer()) ||
                                  colB->CanCollideWith(colA->GetLayer());
                if (!canCollide) continue;

                // AABB交差判定
                AABB aabbA = colA->GetAABB();
                AABB aabbB = colB->GetAABB();

                if (aabbA.Intersects(aabbB)) {
                    currentPairs_.insert({colA, colB});
                }
            }
        }
    }

    // Enter/Stay/Exit イベント発火
    for (const auto& pair : currentPairs_) {
        bool wasColliding = previousPairs_.count(pair) > 0;

        if (!wasColliding) {
            // Enter
            pair.a->InvokeOnEnter(pair.b);
            pair.b->InvokeOnEnter(pair.a);
        }

        // Stay (OnCollision)
        pair.a->InvokeOnCollision(pair.b);
        pair.b->InvokeOnCollision(pair.a);
    }

    // Exit: 前フレームにあって今フレームにない
    for (const auto& pair : previousPairs_) {
        if (currentPairs_.count(pair) == 0) {
            pair.a->InvokeOnExit(pair.b);
            pair.b->InvokeOnExit(pair.a);
        }
    }
}

//----------------------------------------------------------------------------
CollisionManager::Cell CollisionManager::ToCell(float x, float y) const noexcept
{
    return {
        static_cast<int>(std::floor(x / static_cast<float>(cellSize_))),
        static_cast<int>(std::floor(y / static_cast<float>(cellSize_)))
    };
}

//----------------------------------------------------------------------------
void CollisionManager::RebuildGrid()
{
    for (auto& [cell, cellColliders] : grid_) {
        cellColliders.clear();
    }

    for (Collider2D* collider : colliders_) {
        if (!collider->IsColliderEnabled()) continue;

        AABB aabb = collider->GetAABB();

        Cell c0 = ToCell(aabb.minX, aabb.minY);
        Cell c1 = ToCell(aabb.maxX - 0.001f, aabb.maxY - 0.001f);

        for (int cy = c0.y; cy <= c1.y; ++cy) {
            for (int cx = c0.x; cx <= c1.x; ++cx) {
                grid_[{cx, cy}].push_back(collider);
            }
        }
    }
}

//----------------------------------------------------------------------------
// クエリ
//----------------------------------------------------------------------------

void CollisionManager::QueryAABB(const AABB& aabb, std::vector<Collider2D*>& results, uint8_t layerMask)
{
    results.clear();

    Cell c0 = ToCell(aabb.minX, aabb.minY);
    Cell c1 = ToCell(aabb.maxX - 0.001f, aabb.maxY - 0.001f);

    std::unordered_set<Collider2D*> checked;

    for (int cy = c0.y; cy <= c1.y; ++cy) {
        for (int cx = c0.x; cx <= c1.x; ++cx) {
            auto it = grid_.find({cx, cy});
            if (it == grid_.end()) continue;

            for (Collider2D* collider : it->second) {
                if (!collider->IsColliderEnabled()) continue;
                if ((collider->GetLayer() & layerMask) == 0) continue;
                if (checked.count(collider) > 0) continue;
                checked.insert(collider);

                if (aabb.Intersects(collider->GetAABB())) {
                    results.push_back(collider);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------
void CollisionManager::QueryPoint(const Vector2& point, std::vector<Collider2D*>& results, uint8_t layerMask)
{
    results.clear();

    Cell cell = ToCell(point.x, point.y);
    auto it = grid_.find(cell);
    if (it == grid_.end()) return;

    for (Collider2D* collider : it->second) {
        if (!collider->IsColliderEnabled()) continue;
        if ((collider->GetLayer() & layerMask) == 0) continue;

        if (collider->GetAABB().Contains(point.x, point.y)) {
            results.push_back(collider);
        }
    }
}

//----------------------------------------------------------------------------
void CollisionManager::QueryLineSegment(const Vector2& start, const Vector2& end,
                                        std::vector<Collider2D*>& results, uint8_t layerMask)
{
    results.clear();

    float minX = (std::min)(start.x, end.x);
    float maxX = (std::max)(start.x, end.x);
    float minY = (std::min)(start.y, end.y);
    float maxY = (std::max)(start.y, end.y);

    Cell c0 = ToCell(minX, minY);
    Cell c1 = ToCell(maxX, maxY);

    std::unordered_set<Collider2D*> checked;

    for (int cy = c0.y; cy <= c1.y; ++cy) {
        for (int cx = c0.x; cx <= c1.x; ++cx) {
            auto it = grid_.find({cx, cy});
            if (it == grid_.end()) continue;

            for (Collider2D* collider : it->second) {
                if (!collider->IsColliderEnabled()) continue;
                if ((collider->GetLayer() & layerMask) == 0) continue;
                if (checked.count(collider) > 0) continue;
                checked.insert(collider);
            }
        }
    }

    // Liang-Barsky アルゴリズムで線分とAABBの交差判定
    float dx = end.x - start.x;
    float dy = end.y - start.y;

    for (Collider2D* collider : checked) {
        AABB box = collider->GetAABB();

        float tMin = 0.0f;
        float tMax = 1.0f;

        // X軸方向
        if (std::abs(dx) < 1e-8f) {
            if (start.x < box.minX || start.x > box.maxX) continue;
        } else {
            float t1 = (box.minX - start.x) / dx;
            float t2 = (box.maxX - start.x) / dx;
            if (t1 > t2) std::swap(t1, t2);
            tMin = (std::max)(tMin, t1);
            tMax = (std::min)(tMax, t2);
            if (tMin > tMax) continue;
        }

        // Y軸方向
        if (std::abs(dy) < 1e-8f) {
            if (start.y < box.minY || start.y > box.maxY) continue;
        } else {
            float t1 = (box.minY - start.y) / dy;
            float t2 = (box.maxY - start.y) / dy;
            if (t1 > t2) std::swap(t1, t2);
            tMin = (std::max)(tMin, t1);
            tMax = (std::min)(tMax, t2);
            if (tMin > tMax) continue;
        }

        results.push_back(collider);
    }
}

//----------------------------------------------------------------------------
std::optional<RaycastHit> CollisionManager::RaycastFirst(
    const Vector2& start, const Vector2& end, uint8_t layerMask)
{
    std::vector<Collider2D*> hits;
    QueryLineSegment(start, end, hits, layerMask);

    if (hits.empty()) return std::nullopt;

    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float lineLength = std::sqrt(dx * dx + dy * dy);

    std::optional<RaycastHit> closestHit;
    float closestT = 2.0f;

    for (Collider2D* collider : hits) {
        AABB box = collider->GetAABB();

        float tMin = 0.0f;
        float tMax = 1.0f;

        // X軸方向
        if (std::abs(dx) >= 1e-8f) {
            float t1 = (box.minX - start.x) / dx;
            float t2 = (box.maxX - start.x) / dx;
            if (t1 > t2) std::swap(t1, t2);
            tMin = (std::max)(tMin, t1);
            tMax = (std::min)(tMax, t2);
        }

        // Y軸方向
        if (std::abs(dy) >= 1e-8f) {
            float t1 = (box.minY - start.y) / dy;
            float t2 = (box.maxY - start.y) / dy;
            if (t1 > t2) std::swap(t1, t2);
            tMin = (std::max)(tMin, t1);
            tMax = (std::min)(tMax, t2);
        }

        if (tMin < closestT) {
            closestT = tMin;
            RaycastHit hit;
            hit.collider = collider;
            hit.distance = tMin * lineLength;
            hit.point = Vector2(start.x + dx * tMin, start.y + dy * tMin);
            closestHit = hit;
        }
    }

    return closestHit;
}
