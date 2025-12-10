//----------------------------------------------------------------------------
//! @file   test_scene.cpp
//! @brief  テストシーン実装
//----------------------------------------------------------------------------
#include "test_scene.h"
#include "dx11/graphics_context.h"
#include "engine/platform/renderer.h"
#include "engine/platform/application.h"
#include "engine/texture/texture_manager.h"
#include "engine/input/input_manager.h"
#include "engine/graphics2d/sprite_batch.h"
#include "engine/color.h"
#include <cmath>

//----------------------------------------------------------------------------
void TestScene::OnEnter()
{
    auto& app = Application::Get();
    auto* window = app.GetWindow();
    float width = static_cast<float>(window->GetWidth());
    float height = static_cast<float>(window->GetHeight());

    // カメラ作成
    cameraObj_ = std::make_unique<GameObject>("MainCamera");
    camera_ = cameraObj_->AddComponent<Camera2D>(width, height);
    camera_->SetPosition(Vector2(width * 0.5f, height * 0.5f));

    // テスト用白テクスチャを作成（32x32）
    std::vector<uint32_t> whitePixels(32 * 32, 0xFFFFFFFF);
    testTexture_ = TextureManager::Get().Create2D(
        32, 32,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D11_BIND_SHADER_RESOURCE,
        whitePixels.data(),
        32 * sizeof(uint32_t)
    );

    // テストオブジェクトを複数作成
    for (int i = 0; i < 5; ++i) {
        auto obj = std::make_unique<GameObject>("Sprite" + std::to_string(i));

        auto* transform = obj->AddComponent<Transform2D>();
        transform->SetPosition(Vector2(200.0f + i * 150.0f, 300.0f));
        transform->SetScale(2.0f);

        auto* sprite = obj->AddComponent<SpriteRenderer>();
        sprite->SetTexture(testTexture_.get());
        sprite->SetColor(ColorFromHSV(i * 72.0f, 0.8f, 1.0f));  // 異なる色

        objects_.push_back(std::move(obj));
    }

    // 親子関係テスト：最初のオブジェクトを親に
    if (objects_.size() >= 2) {
        auto* parentTransform = objects_[0]->GetComponent<Transform2D>();
        auto* childTransform = objects_[1]->GetComponent<Transform2D>();
        childTransform->SetParent(parentTransform);
        childTransform->SetPosition(Vector2(50, 50));  // 親からの相対位置
    }

    // SpriteBatch初期化
    SpriteBatch::Get().Initialize();
}

//----------------------------------------------------------------------------
void TestScene::OnExit()
{
    objects_.clear();
    cameraObj_.reset();
    testTexture_.reset();
    SpriteBatch::Get().Shutdown();
}

//----------------------------------------------------------------------------
void TestScene::Update()
{
    float dt = Application::Get().GetDeltaTime();
    time_ += dt;

    auto* inputMgr = InputManager::GetInstance();
    if (!inputMgr) return;

    auto& keyboard = inputMgr->GetKeyboard();

    // カメラ操作
    // WASD: 移動
    Vector2 cameraMove = Vector2::Zero;
    if (keyboard.IsKeyPressed(Key::W)) cameraMove.y -= 200.0f * dt;
    if (keyboard.IsKeyPressed(Key::S)) cameraMove.y += 200.0f * dt;
    if (keyboard.IsKeyPressed(Key::A)) cameraMove.x -= 200.0f * dt;
    if (keyboard.IsKeyPressed(Key::D)) cameraMove.x += 200.0f * dt;
    camera_->Translate(cameraMove);

    // Q/E: ズーム
    if (keyboard.IsKeyPressed(Key::Q)) camera_->SetZoom(camera_->GetZoom() * (1.0f - dt));
    if (keyboard.IsKeyPressed(Key::E)) camera_->SetZoom(camera_->GetZoom() * (1.0f + dt));

    // R: リセット
    if (keyboard.IsKeyDown(Key::R)) {
        auto* window = Application::Get().GetWindow();
        camera_->SetPosition(Vector2(window->GetWidth() * 0.5f, window->GetHeight() * 0.5f));
        camera_->SetZoom(1.0f);
        camera_->SetRotation(0.0f);
    }

    // 親オブジェクトを回転（子も一緒に回転する）
    if (!objects_.empty()) {
        auto* parentTransform = objects_[0]->GetComponent<Transform2D>();
        parentTransform->Rotate(dt);  // 毎秒約57度回転
    }

    // 全オブジェクト更新
    for (auto& obj : objects_) {
        obj->Update(dt);
    }
}

//----------------------------------------------------------------------------
void TestScene::Render()
{
    auto& ctx = GraphicsContext::Get();
    auto& renderer = Renderer::Get();

    Texture* backBuffer = renderer.GetBackBuffer();
    if (!backBuffer) return;

    ctx.SetRenderTarget(backBuffer, nullptr);
    ctx.SetViewport(0, 0,
        static_cast<float>(backBuffer->Width()),
        static_cast<float>(backBuffer->Height()));

    // 背景クリア
    float clearColor[4] = { 0.1f, 0.1f, 0.2f, 1.0f };
    ctx.ClearRenderTarget(backBuffer, clearColor);

    // SpriteBatchで描画
    auto& spriteBatch = SpriteBatch::Get();
    spriteBatch.SetCamera(*camera_);
    spriteBatch.Begin();

    for (auto& obj : objects_) {
        auto* transform = obj->GetComponent<Transform2D>();
        auto* sprite = obj->GetComponent<SpriteRenderer>();
        if (transform && sprite) {
            spriteBatch.Draw(*sprite, *transform);
        }
    }

    spriteBatch.End();
}
