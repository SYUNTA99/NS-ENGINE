#include "input_manager.h"
#include "engine/core/singleton_registry.h"

InputManager::InputManager() noexcept
    : keyboard_(std::make_unique<Keyboard>())
    , mouse_(std::make_unique<Mouse>())
    , gamepadManager_(std::make_unique<GamepadManager>())
{
}

void InputManager::Create()
{
    if (!instance_) {
        instance_ = std::unique_ptr<InputManager>(new InputManager());
        SINGLETON_REGISTER(InputManager, SingletonId::None);
    }
}

void InputManager::Destroy()
{
    if (instance_) {
        SINGLETON_UNREGISTER(InputManager);
        instance_.reset();
    }
}

void InputManager::Update(float deltaTime) noexcept
{
    if (keyboard_) {
        keyboard_->Update(deltaTime);
    }
    if (mouse_) {
        mouse_->Update();
    }
   //if (gamepadManager_) {
   //     gamepadManager_->Update();
   // }
}
