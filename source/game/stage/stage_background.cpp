//----------------------------------------------------------------------------
//! @file   stage_background.cpp
//! @brief  ステージ背景実装
//----------------------------------------------------------------------------
#include "stage_background.h"
#include "engine/texture/texture_manager.h"
#include "engine/shader/shader_manager.h"
#include "engine/c_systems/sprite_batch.h"
#include "dx11/gpu/texture.h"
#include "common/logging/logging.h"
#include <algorithm>

//----------------------------------------------------------------------------
void StageBackground::Initialize(const std::string& stageId, float screenWidth, float screenHeight)
{
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    stageWidth_ = screenWidth;
    stageHeight_ = screenHeight;

    // 乱数初期化
    std::random_device rd;
    rng_ = std::mt19937(rd());

    // ベースカラー（草原の緑 - 明るめ）
    baseColor_ = Color(0.45f, 0.65f, 0.40f, 1.0f);

    // 1x1白テクスチャを作成（ベースカラー描画用）
    {
        uint32_t whitePixel = 0xFFFFFFFF;  // RGBA白
        whiteTexture_ = Texture::Create2D(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &whitePixel);
    }

    // テクスチャパスのベース
    std::string basePath = stageId + "/";

    // 地面テクスチャ読み込み
    groundTexture_ = TextureManager::Get().LoadTexture2D(basePath + "ground.png");
    if (groundTexture_) {
        float texW = static_cast<float>(groundTexture_->Width());
        float texH = static_cast<float>(groundTexture_->Height());

        // タイルサイズ（テクスチャ全体を使用）
        tileWidth_ = texW;
        tileHeight_ = texH;

        // オーバーラップ率（fadeWidth=0.15 × 2 = 30%が正しい）
        float overlapRatio = 0.30f;
        float stepX = tileWidth_ * (1.0f - overlapRatio);
        float stepY = tileHeight_ * (1.0f - overlapRatio);

        // ランダム回転（0, 90, 180, 270度）
        std::uniform_int_distribution<int> rotDist(0, 3);
        std::uniform_int_distribution<int> flipDist(0, 1);

        // ステージ全体をカバーするタイル数を計算（オーバーラップ考慮）
        int tilesX = static_cast<int>(std::ceil(screenWidth / stepX)) + 2;
        int tilesY = static_cast<int>(std::ceil(screenHeight / stepY)) + 2;

        // タイルを配置（オーバーラップあり、シェーダーで端フェード）
        for (int y = -1; y < tilesY; ++y) {
            for (int x = -1; x < tilesX; ++x) {
                GroundTile tile;
                tile.position = Vector2(
                    x * stepX + tileWidth_ * 0.5f,
                    y * stepY + tileHeight_ * 0.5f
                );
                tile.rotation = rotDist(rng_) * 1.5707963f;  // 0, 90, 180, 270度
                tile.flipX = flipDist(rng_) == 1;
                tile.flipY = flipDist(rng_) == 1;
                tile.alpha = 1.0f;  // フルアルファ（シェーダーで端フェード）
                groundTiles_.push_back(tile);
            }
        }

        LOG_INFO("[StageBackground] Ground tiles: " + std::to_string(groundTiles_.size()) + " (edge fade shader + overlap)");
    } else {
        LOG_ERROR("[StageBackground] Failed to load ground texture: " + basePath + "ground.png");
    }

    // 地面用シェーダー読み込み（端フェード付き）
    groundVertexShader_ = ShaderManager::Get().LoadVertexShader("ground_vs.hlsl");
    groundPixelShader_ = ShaderManager::Get().LoadPixelShader("ground_ps.hlsl");
    if (groundVertexShader_ && groundPixelShader_) {
        LOG_INFO("[StageBackground] Ground shaders loaded");
    } else {
        LOG_WARN("[StageBackground] Ground shaders not loaded, using default");
    }

    // 装飾を配置
    PlaceDecorations(stageId, screenWidth, screenHeight);

    LOG_INFO("[StageBackground] Initialized with " + std::to_string(decorations_.size()) + " decorations");
}

//----------------------------------------------------------------------------
void StageBackground::PlaceDecorations(const std::string& stageId, float screenWidth, float screenHeight)
{
    std::string basePath = stageId + "/";

    // 分布設定
    std::uniform_real_distribution<float> xDist(0.0f, screenWidth);
    std::uniform_real_distribution<float> yFullDist(screenHeight * 0.3f, screenHeight);
    std::uniform_real_distribution<float> yGroundDist(screenHeight * 0.6f, screenHeight * 0.95f);
    std::uniform_real_distribution<float> scaleDist(0.8f, 1.2f);
    std::uniform_real_distribution<float> smallScaleDist(0.5f, 1.0f);
    std::uniform_real_distribution<float> rotationDist(-0.1f, 0.1f);
    std::uniform_int_distribution<int> countDist5_8(5, 8);
    std::uniform_int_distribution<int> countDist10_15(10, 15);
    std::uniform_int_distribution<int> countDist15_25(15, 25);

    // 遺跡・木（奥レイヤー -120）
    {
        std::vector<std::string> bigObjects = {
            "ruins fragment.png",
            "ruins fragment 2.png",
            "ruins fragment 3.png",
            "tree.png"
        };

        int count = countDist5_8(rng_);
        std::uniform_int_distribution<size_t> objDist(0, bigObjects.size() - 1);

        for (int i = 0; i < count; ++i) {
            TexturePtr tex = TextureManager::Get().LoadTexture2D(basePath + bigObjects[objDist(rng_)]);
            if (tex) {
                Vector2 pos(xDist(rng_), yFullDist(rng_));
                Vector2 scale(scaleDist(rng_), scaleDist(rng_));
                AddDecoration(tex, pos, -90, scale, rotationDist(rng_));
            }
        }
    }

    // 草・石（中レイヤー -100）
    {
        std::vector<std::string> mediumObjects = {
            "grass big.png",
            "grass long.png",
            "stone 1.png",
            "stone 2.png",
            "stone 3.png",
            "stone 4.png",
            "stone 5.png",
            "stone 6.png",
            "stone 7.png",
            "stone 8.png"
        };

        int count = countDist10_15(rng_);
        std::uniform_int_distribution<size_t> objDist(0, mediumObjects.size() - 1);

        for (int i = 0; i < count; ++i) {
            TexturePtr tex = TextureManager::Get().LoadTexture2D(basePath + mediumObjects[objDist(rng_)]);
            if (tex) {
                Vector2 pos(xDist(rng_), yGroundDist(rng_));
                Vector2 scale(scaleDist(rng_), scaleDist(rng_));
                AddDecoration(tex, pos, -100, scale, rotationDist(rng_));
            }
        }
    }

    // 葉・木片・焚火・小さい草（手前レイヤー -80）
    {
        std::vector<std::string> smallObjects = {
            "grass small.png",
            "leaf 1.png",
            "leaf 2.png",
            "leaf 3.png",
            "leaf 4.png",
            "leaf 5.png",
            "leaf 6.png",
            "leaf 7.png",
            "leaf 8.png",
            "wood chips 1.png",
            "wood chips 2.png",
            "wood chips 3.png",
            "wood chips 4.png",
            "wood chips 5.png",
            "wood chips 6.png"
        };

        int count = countDist15_25(rng_);
        std::uniform_int_distribution<size_t> objDist(0, smallObjects.size() - 1);

        for (int i = 0; i < count; ++i) {
            TexturePtr tex = TextureManager::Get().LoadTexture2D(basePath + smallObjects[objDist(rng_)]);
            if (tex) {
                Vector2 pos(xDist(rng_), yFullDist(rng_));
                Vector2 scale(smallScaleDist(rng_), smallScaleDist(rng_));
                AddDecoration(tex, pos, -80, scale, rotationDist(rng_));
            }
        }

        // 焚火（1つだけ、画面中央付近）
        TexturePtr bonfire = TextureManager::Get().LoadTexture2D(basePath + "bonfire.png");
        if (bonfire) {
            Vector2 pos(screenWidth * 0.5f + xDist(rng_) * 0.2f - screenWidth * 0.1f,
                        screenHeight * 0.75f);
            AddDecoration(bonfire, pos, -80, Vector2::One, 0.0f);
        }
    }
}

//----------------------------------------------------------------------------
void StageBackground::AddDecoration(TexturePtr texture, const Vector2& position,
                                     int sortingLayer, const Vector2& scale, float rotation)
{
    DecorationObject obj;
    obj.texture = texture;
    obj.position = position;
    obj.scale = scale;
    obj.rotation = rotation;
    obj.sortingLayer = sortingLayer;
    decorations_.push_back(std::move(obj));
}

//----------------------------------------------------------------------------
void StageBackground::Render(SpriteBatch& spriteBatch)
{
    // 1. ベースカラー（単色の緑）を描画
    if (whiteTexture_) {
        Vector2 baseScale(stageWidth_, stageHeight_);
        spriteBatch.Draw(
            whiteTexture_.get(),
            Vector2(stageWidth_ * 0.5f, stageHeight_ * 0.5f),
            baseColor_,
            0.0f,
            Vector2(0.5f, 0.5f),
            baseScale,
            false, false,
            -99, 0
        );
    }

    // ベースを先にフラッシュ
    spriteBatch.End();

    // 2. 地面タイル（端フェードシェーダーで描画）
    if (groundTexture_ && groundVertexShader_ && groundPixelShader_) {
        spriteBatch.SetCustomShaders(groundVertexShader_.get(), groundPixelShader_.get());
        spriteBatch.Begin();

        Vector2 origin(tileWidth_ * 0.5f, tileHeight_ * 0.5f);
        for (const GroundTile& tile : groundTiles_) {
            spriteBatch.Draw(
                groundTexture_.get(),
                tile.position,
                Color(1.0f, 1.0f, 1.0f, tile.alpha),
                tile.rotation,
                origin,
                Vector2::One,
                tile.flipX, tile.flipY,
                -98, 0
            );
        }

        spriteBatch.End();
        spriteBatch.ClearCustomShaders();
    }

    // 3. 装飾描画
    spriteBatch.Begin();
    for (const DecorationObject& obj : decorations_) {
        if (!obj.texture) continue;

        float texW = static_cast<float>(obj.texture->Width());
        float texH = static_cast<float>(obj.texture->Height());
        Vector2 origin(texW * 0.5f, texH * 0.5f);

        spriteBatch.Draw(
            obj.texture.get(),
            obj.position,
            Color(1.0f, 1.0f, 1.0f, 1.0f),
            obj.rotation,
            origin,
            obj.scale,
            false, false,
            obj.sortingLayer, 0
        );
    }
}

//----------------------------------------------------------------------------
void StageBackground::Shutdown()
{
    groundTiles_.clear();
    decorations_.clear();
    groundTexture_.reset();
    whiteTexture_.reset();
    groundVertexShader_.reset();
    groundPixelShader_.reset();

    LOG_INFO("[StageBackground] Shutdown");
}
