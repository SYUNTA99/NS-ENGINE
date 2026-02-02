//----------------------------------------------------------------------------
//! @file   application.inl
//! @brief  アプリケーションテンプレート実装
//----------------------------------------------------------------------------
#pragma once

#include "renderer.h"
#include "common/logging/logging.h"

//----------------------------------------------------------------------------
template<typename TGame>
void Application::Run(TGame& game)
{
    if (running_) {
        LOG_WARN("[Application] 既に実行中です");
        return;
    }
    running_ = true;
    shouldQuit_ = false;

    MainLoop(game);

    running_ = false;
}

//----------------------------------------------------------------------------
template<typename TGame>
void Application::MainLoop(TGame& game)
{
    Renderer& renderer = Renderer::Get();
    const float fixedDt = desc_.fixedDeltaTime;
    const bool useFixed = desc_.useFixedTimestep;
    accumulator_ = 0.0f;

    while (!shouldQuit_) {
        // メッセージ処理
        if (!window_->ProcessMessages()) {
            break;  // WM_QUIT
        }

        if (window_->ShouldClose()) {
            break;
        }

        // 最小化中はスリープ
        if (window_->IsMinimized()) {
            Sleep(10);
            continue;
        }

        // 時間更新
        Timer::Update(desc_.maxDeltaTime);
        const float dt = Timer::GetDeltaTime();

        // 入力処理
        ProcessInput();

        if (useFixed) {
            // 固定タイムステップ（物理・ロジック）
            accumulator_ += dt;

            // スパイク防止: 最大5回まで
            int iterations = 0;
            constexpr int kMaxIterations = 5;

            while (accumulator_ >= fixedDt && iterations < kMaxIterations) {
                game.FixedUpdate(fixedDt);
                accumulator_ -= fixedDt;
                ++iterations;
            }

            // 残りが多すぎる場合は切り捨て（デバッグ時のブレークポイント対策）
            if (accumulator_ > fixedDt * 2.0f) {
                accumulator_ = fixedDt;
            }

            // 描画補間係数を計算
            alpha_ = accumulator_ / fixedDt;
        } else {
            // 可変タイムステップ（従来互換）
            game.Update();
            alpha_ = 1.0f;
        }

        // 描画
        game.Render();

        // Present
        renderer.Present();

        // フレーム末処理
        game.EndFrame();
    }
}
