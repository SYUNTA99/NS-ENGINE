//----------------------------------------------------------------------------
//! @file   collider2d.cpp
//! @brief  2D当たり判定コンポーネント実装
//----------------------------------------------------------------------------

#include "collider2d.h"
#include "transform.h"
#include "game_object.h"
#include "engine/c_systems/collision_manager.h"

//----------------------------------------------------------------------------
void Collider2D::OnAttach()
{
    CollisionManager::Get().Register(this);
}

//----------------------------------------------------------------------------
void Collider2D::OnDetach()
{
    CollisionManager::Get().Unregister(this);
}

//----------------------------------------------------------------------------
void Collider2D::Update([[maybe_unused]] float deltaTime)
{
    // Transformと自動同期
    if (syncWithTransform_) {
        if (GameObject* owner = GetOwner()) {
            if (Transform* transform = owner->GetComponent<Transform>()) {
                Vector3 pos = transform->GetWorldPosition();
                position_ = Vector2(pos.x, pos.y);
            }
        }
    }
}
