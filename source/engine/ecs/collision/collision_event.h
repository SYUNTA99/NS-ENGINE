//----------------------------------------------------------------------------
//! @file   collision_event.h
//! @brief  衝突イベント構造体
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/actor.h"
#include "engine/math/math_types.h"
#include <cstdint>

namespace Collision {

//============================================================================
//! @brief 衝突イベントタイプ
//============================================================================
enum class EventType : uint8_t {
    Enter = 0,   //!< 衝突開始
    Stay = 1,    //!< 衝突継続
    Exit = 2     //!< 衝突終了
};

//============================================================================
//! @brief 2D衝突イベント（POD - 32 bytes）
//============================================================================
struct alignas(16) Event2D {
    ECS::Actor actorA;               //!< コライダーA (4 bytes)
    ECS::Actor actorB;               //!< コライダーB (4 bytes)
    float contactX = 0.0f;           //!< 接触点X (4 bytes)
    float contactY = 0.0f;           //!< 接触点Y (4 bytes)
    float normalX = 0.0f;            //!< 接触法線X (4 bytes)
    float normalY = 0.0f;            //!< 接触法線Y (4 bytes)
    float penetration = 0.0f;        //!< 侵入深度 (4 bytes)
    EventType type = EventType::Enter;  //!< イベントタイプ (1 byte)
    uint8_t layerA = 0;              //!< AのレイヤーID (1 byte)
    uint8_t layerB = 0;              //!< BのレイヤーID (1 byte)
    uint8_t _pad = 0;                //!< パディング (1 byte)

    //! @brief 指定Actorが関与しているか
    [[nodiscard]] bool Involves(ECS::Actor actor) const noexcept {
        return actorA == actor || actorB == actor;
    }

    //! @brief 相手のActorを取得
    [[nodiscard]] ECS::Actor GetOther(ECS::Actor self) const noexcept {
        return (actorA == self) ? actorB : actorA;
    }
};

//============================================================================
//! @brief 3D衝突イベント（POD - 48 bytes）
//============================================================================
struct alignas(16) Event3D {
    ECS::Actor actorA;               //!< コライダーA (4 bytes)
    ECS::Actor actorB;               //!< コライダーB (4 bytes)
    float contactX = 0.0f;           //!< 接触点X (4 bytes)
    float contactY = 0.0f;           //!< 接触点Y (4 bytes)
    float contactZ = 0.0f;           //!< 接触点Z (4 bytes)
    float _pad0 = 0.0f;              //!< アライメント (4 bytes)
    float normalX = 0.0f;            //!< 接触法線X (4 bytes)
    float normalY = 0.0f;            //!< 接触法線Y (4 bytes)
    float normalZ = 0.0f;            //!< 接触法線Z (4 bytes)
    float penetration = 0.0f;        //!< 侵入深度 (4 bytes)
    EventType type = EventType::Enter;  //!< イベントタイプ (1 byte)
    uint8_t _pad1[3] = {};           //!< パディング (3 bytes)
    uint32_t layerA = 0;             //!< AのレイヤーID (4 bytes)
    uint32_t layerB = 0;             //!< BのレイヤーID (4 bytes)

    //! @brief 指定Actorが関与しているか
    [[nodiscard]] bool Involves(ECS::Actor actor) const noexcept {
        return actorA == actor || actorB == actor;
    }

    //! @brief 相手のActorを取得
    [[nodiscard]] ECS::Actor GetOther(ECS::Actor self) const noexcept {
        return (actorA == self) ? actorB : actorA;
    }

    //! @brief 接触点をVector3で取得
    [[nodiscard]] Vector3 GetContactPoint() const noexcept {
        return Vector3(contactX, contactY, contactZ);
    }

    //! @brief 接触法線をVector3で取得
    [[nodiscard]] Vector3 GetNormal() const noexcept {
        return Vector3(normalX, normalY, normalZ);
    }
};

} // namespace Collision
