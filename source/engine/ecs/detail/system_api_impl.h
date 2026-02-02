//----------------------------------------------------------------------------
//! @file   system_api_impl.h
//! @brief  SystemAPI テンプレート実装
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system_api.h"
#include "engine/ecs/world.h"

namespace ECS {

//----------------------------------------------------------------------------
inline ActorRegistry& SystemAPI::EntityManager() {
    assert(currentState_ && currentState_->world && "SystemAPI::SetCurrentState() must be called first");
    return currentState_->world->Actors();
}

//----------------------------------------------------------------------------
template<typename... AccessModes>
auto SystemAPI::Query() {
    assert(currentState_ && currentState_->world && "SystemAPI::SetCurrentState() must be called first");
    return currentState_->world->Actors().Query<AccessModes...>();
}

//----------------------------------------------------------------------------
inline Actor SystemAPI::CreateActor() {
    assert(currentState_ && currentState_->world && "SystemAPI::SetCurrentState() must be called first");
    return currentState_->world->CreateActor();
}

//----------------------------------------------------------------------------
inline void SystemAPI::DestroyActor(Actor actor) {
    assert(currentState_ && currentState_->world && "SystemAPI::SetCurrentState() must be called first");
    currentState_->world->DestroyActor(actor);
}

//----------------------------------------------------------------------------
template<typename T>
T* SystemAPI::GetComponent(Actor actor) {
    assert(currentState_ && currentState_->world && "SystemAPI::SetCurrentState() must be called first");
    return currentState_->world->GetComponent<T>(actor);
}

//----------------------------------------------------------------------------
template<typename T, typename... Args>
T* SystemAPI::AddComponent(Actor actor, Args&&... args) {
    assert(currentState_ && currentState_->world && "SystemAPI::SetCurrentState() must be called first");
    return currentState_->world->AddComponent<T>(actor, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------
template<typename T>
bool SystemAPI::HasComponent(Actor actor) {
    assert(currentState_ && currentState_->world && "SystemAPI::SetCurrentState() must be called first");
    return currentState_->world->HasComponent<T>(actor);
}

//----------------------------------------------------------------------------
template<typename T>
void SystemAPI::RemoveComponent(Actor actor) {
    assert(currentState_ && currentState_->world && "SystemAPI::SetCurrentState() must be called first");
    currentState_->world->RemoveComponent<T>(actor);
}

} // namespace ECS
