# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NS-ENGINE is a Windows game engine built for the HEW game development contest by team "Nihonium113". It uses C++20, DirectX 11, and Visual Studio 2022.

## Build Commands

```bash
# Full rebuild (clean, generate VS solution, build debug)
@make_project.cmd

# Build only (Debug or Release)
tools\build.cmd [Debug|Release]

# Run tests
tools\run_tests.cmd [Debug|Release]

# Open in Visual Studio
@open_project.cmd

# Clean build artifacts
@cleanup.cmd

# Regenerate VS solution only (after adding/removing files)
tools\regen_project.cmd
```

**Output location:** `build/bin/{Debug,Release,Burst}-windows-x86_64/game/game.exe`

**Build system:** Premake5 generates Visual Studio 2022 solutions. Configuration is in `premake5.lua`.

**Build configurations:**
- **Debug**: Full symbols, no optimization
- **Release**: Full optimization with LTO
- **Burst**: Maximum optimization (slow compile, fast runtime)

## Architecture

### Project Structure (dependency order)
1. **common** - Header-only utilities (logging, non_copyable)
2. **dx11** - DirectX 11 wrapper (StaticLib) - D3D11 device, buffers, textures, shaders, pipeline states
3. **engine** - Game engine layer (StaticLib) - ECS, scenes, input, resources, rendering
4. **game** - Game executable (WindowedApp)

### Entity-Component-System (ECS)

The engine provides a data-oriented ECS with SoA (Structure of Arrays) storage for cache-efficient iteration.

**Core Types:**
- `ECS::World` - Central container for actors, components, and systems
- `ECS::Actor` - Lightweight 32-bit ID (20-bit index + 12-bit generation)
- `ComponentStorage<T>` - SoA storage for component data

**World-centric API (preferred for performance):**
```cpp
ECS::World world;

// Create actor and add components
auto actor = world.CreateActor();
world.AddComponent<TransformData>(actor, position, rotation, scale);
world.AddComponent<SpriteData>(actor, textureHandle);

// Query components
auto* transform = world.GetComponent<TransformData>(actor);

// Batch iteration (cache-friendly)
world.ForEach<TransformData, SpriteData>([](Actor e, TransformData& t, SpriteData& s) {
    // Process all actors with both components
});
```

**System Registration:**
```cpp
class MySpriteSystem final : public ISystem {
    int Priority() const override { return 100; }
    void Execute(World& world, float dt) override {
        world.ForEach<TransformData, SpriteData>([](Actor e, auto& t, auto& s) {
            // Update logic
        });
    }
};

world.RegisterSystem<MySpriteSystem>();
world.RegisterRenderSystem<SpriteRenderSystem>();
```

**OOP API (legacy compatibility):**
```cpp
// GameObject wrapper around Actor (for traditional OOP style)
auto* go = world.CreateGameObject("Player");
go->AddComponent<SpriteRenderer>(...);
```

### Singletons via ServiceLocator

```cpp
// Access managers through Services (preferred for testability)
Services::Textures().Load(...)
Services::Jobs().Schedule(...)
Services::Input().IsKeyPressed(...)
Services::Collision2D().CheckCollision(...)
Services::Shaders().Load(...)
Services::FileSystem().ReadFile(...)

// Or direct singleton access
JobSystem::Get().Schedule(priority, [](CancelTokenPtr token) { ... });
SceneManager::Get().ChangeScene<MyScene>();
```

**Singleton Lifecycle with SingletonRegistry:**
```cpp
// In Create() - register with dependencies
SINGLETON_REGISTER(TextureManager,
    SingletonId::GraphicsDevice | SingletonId::GraphicsContext);

// In Destroy() - unregister
SINGLETON_UNREGISTER(TextureManager);
```
The registry validates initialization order and asserts in Debug builds if dependencies aren't met.

### Scene Lifecycle

```cpp
class MyScene : public Scene {
    void OnEnter() override {
        InitializeWorld();  // Create ECS World
        GetWorldRef().RegisterSystem<MySpriteSystem>();
    }
    void OnExit() override;       // Cleanup
    void OnLoadAsync() override;  // Background loading (thread-safe)
    void FixedUpdate(float dt) override;  // Physics/logic (calls World::FixedUpdate)
    void Render(float alpha) override;    // Drawing (calls World::Render)
};
```

**File System Mounts:**
```cpp
FileSystemManager::Get().Mount("shaders", std::make_unique<HostFileSystem>(L"assets/shader/"));
auto data = FileSystemManager::Get().ReadFile("shaders:/vs.hlsl");
```

### Initialization Order (Game::Initialize)
1. Core: `JobSystem`, `InputManager`, `FileSystemManager`
2. Graphics: `ShaderManager`, `RenderStateManager`
3. Rendering: `SpriteBatch`, `MeshBatch`
4. Systems: `CollisionManager`, `MeshManager`, `MaterialManager`, `LightingManager`, `SceneManager`
5. Mount points for shaders/, textures/, stages/, models/
6. Manager initialization calls
7. Initial scene load

### Resource Cleanup
Clear D3D state before releasing resources:
```cpp
ctx->ClearState();
ctx->Flush();
// Then destroy in reverse init order
```

## External Dependencies

- **DirectXTex/DirectXTK:** Pre-built in `external/lib/{Debug,Release}/`
- **Assimp 6.0.2:** DLLs auto-copied via post-build (`assimp-vc143-mt{d}.dll`)
- **tinygltf:** Header-only via NuGet (`packages/tinygltf/`)
- **Google Test 1.15.2:** Source in `external/googletest/` (tests only)

## Code Conventions

- **Strict warnings:** `/W4 /WX` - all warnings are errors
- **Encoding:** `/utf-8`
- **Naming:** PascalCase classes, camelCase functions, UPPER_CASE constants, snake_case_ private members
- **COM objects:** Use `Microsoft::WRL::ComPtr`
- **Pre-build checks:**
  - `tools/check_raw_view_comptr.cmd` - validates ComPtr usage in dx11/
  - `tools/check_rh_functions.cmd` - prevents right-hand coordinate system functions (engine uses left-hand)

## Git Workflow

- **master直接push禁止** - Always create PRs
- Branch naming: `feature/`, `fix/`, `refactor/`
- CodeRabbit provides automatic code review
- PRのタイトルとコミットメッセージは日本語で

## Testing

```bash
# Build and run all tests
tools\run_tests.cmd

# Run specific test (after build)
build\bin\Debug-windows-x86_64\tests\tests.exe --gtest_filter=TestName*
```

- **Framework:** Google Test (v1.15.2) in `external/googletest/`
- **Test files:** `source/tests/` directory (`*_test.cpp`)

## Threading

- D3D11 device is thread-safe after initialization
- Use `JobSystem` for async work with priority levels (High/Normal/Low)
- Main thread lifecycle: `BeginFrame()` → `FixedUpdate()` → `Update()` → `Render()` → `EndFrame()`
- Cancel long tasks via `CancelTokenPtr`
