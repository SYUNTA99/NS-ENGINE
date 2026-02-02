//----------------------------------------------------------------------------
//! @file   result_scene.h
//! @brief  リザルトシーン
//----------------------------------------------------------------------------
#pragma once
#include "engine/scene/scene.h"
#include "engine/scene/scene_manager.h"
#include "engine/input/input_manager.h"
#include "engine/platform/renderer.h"
#include "engine/graphics/sprite_batch.h"
#include "engine/texture/texture_manager.h"
#include "dx11/graphics_context.h"
#include "common/logging/logging.h"

//============================================================================
//! @brief リザルトシーン
//============================================================================
class ResultScene : public Scene
{
public:
    [[nodiscard]] const char* GetName() const override { return "ResultScene"; }

    void OnEnter() override
    {
        LOG_INFO("[ResultScene] Enter - Game Clear!");

        // リザルトロゴをロード
        resultLogo_ = TextureManager::Get().Load("texture:/resultlog.png");
        if (resultLogo_) {
            LOG_INFO("[ResultScene] Result logo loaded");
        }
    }

    void OnExit() override
    {
        LOG_INFO("[ResultScene] Exit");
    }

    void FixedUpdate(float dt) override
    {
        (void)dt;
        auto& keyboard = InputManager::Get().GetKeyboard();

        // SpaceまたはEnterでタイトルに戻る
        if (keyboard.IsKeyPressed(Key::Space) || keyboard.IsKeyPressed(Key::Enter)) {
            LOG_INFO("[ResultScene] Returning to title...");
            SceneManager::Get().Load<TitleScene>();
        }
    }

    void Render([[maybe_unused]] float alpha) override
    {
        GraphicsContext& ctx = GraphicsContext::Get();
        Renderer& renderer = Renderer::Get();

        Texture* backBuffer = renderer.GetBackBuffer();
        Texture* depthBuffer = renderer.GetDepthBuffer();
        if (!backBuffer || !depthBuffer) return;

        float width = static_cast<float>(backBuffer->Width());
        float height = static_cast<float>(backBuffer->Height());

        ctx.SetRenderTarget(backBuffer, depthBuffer);
        ctx.SetViewport(0, 0, width, height);

        // 緑の背景（クリア感）
        float clearColor[4] = { 0.1f, 0.4f, 0.2f, 1.0f };
        ctx.ClearRenderTarget(backBuffer, clearColor);
        ctx.ClearDepthStencil(depthBuffer, 1.0f, 0);

        // リザルトロゴを描画（中央）
        Texture* logoTex = TextureManager::Get().Get(resultLogo_);
        if (logoTex) {
            SpriteBatch& sb = SpriteBatch::Get();
            sb.Begin();

            float logoWidth = static_cast<float>(logoTex->Width());
            float logoHeight = static_cast<float>(logoTex->Height());

            // スケール調整（画面幅の60%程度に収める）
            float scale = (width * 0.6f) / logoWidth;
            float scaledWidth = logoWidth * scale;
            float scaledHeight = logoHeight * scale;

            // 中央に配置
            float x = (width - scaledWidth) * 0.5f;
            float y = (height - scaledHeight) * 0.5f;

            sb.Draw(logoTex, Vector2(x, y), Colors::White, 0.0f, Vector2::Zero, Vector2(scale, scale));
            sb.End();
        }
    }

private:
    TextureHandle resultLogo_;
};
