//----------------------------------------------------------------------------
//! @file   system_state_impl.h
//! @brief  SystemState テンプレート実装
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system_state.h"
#include "engine/ecs/world.h"

namespace ECS {

//----------------------------------------------------------------------------
inline ActorRegistry& SystemState::EntityManager() {
    return world->Actors();
}

inline const ActorRegistry& SystemState::EntityManager() const {
    return world->Actors();
}

//----------------------------------------------------------------------------
inline Actor SystemState::CreateActor() {
    return world->CreateActor();
}

//----------------------------------------------------------------------------
inline void SystemState::DestroyActor(Actor actor) {
    world->DestroyActor(actor);
}

//----------------------------------------------------------------------------
template<typename T>
T* SystemState::GetComponent(Actor actor) {
    return world->GetComponent<T>(actor);
}

template<typename T>
const T* SystemState::GetComponent(Actor actor) const {
    return world->GetComponent<T>(actor);
}

//----------------------------------------------------------------------------
template<typename T, typename... Args>
T* SystemState::AddComponent(Actor actor, Args&&... args) {
    return world->AddComponent<T>(actor, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------
template<typename T>
bool SystemState::HasComponent(Actor actor) const {
    return world->HasComponent<T>(actor);
}

//----------------------------------------------------------------------------
template<typename T>
void SystemState::RemoveComponent(Actor actor) {
    world->RemoveComponent<T>(actor);
}

} // namespace ECS
