//----------------------------------------------------------------------------
//! @file   sprite_render_system.h
//! @brief  ECS SpriteRenderSystem - スプライト描画システム
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/transform_components.h"
#include "engine/ecs/components/rendering/sprite_data.h"
#include "engine/ecs/components/animation/animator_data.h"
#include "engine/graphics/sprite_batch.h"
#include "engine/texture/texture_manager.h"

namespace ECS {

//============================================================================
//! @brief スプライト描画システム（描画システム）
//!
//! 入力: LocalToWorld, SpriteData（読み取り専用）
//! 出力: GPU (SpriteBatch)
//!
//! @note 優先度0（描画システムの中で最初に実行）
//============================================================================
class SpriteRenderSystem final : public IRenderSystem {
public:
    void OnRender(World& world, [[maybe_unused]] float alpha) override {
        // AnimatorData → SpriteData UV同期
        world.ForEach<SpriteData, AnimatorData>(
            [](Actor, SpriteData& sprite, AnimatorData& anim) {
                sprite.uvOffset = anim.GetUVOffset();
                sprite.uvSize = anim.GetUVSize();
            });

        auto& batch = SpriteBatch::Get();
        auto& texMgr = TextureManager::Get();

        batch.Begin();

        // LocalToWorldとSpriteDataを持つ全エンティティを描画
        world.ForEach<LocalToWorld, SpriteData>(
            [&batch, &texMgr, &world](Actor actor, LocalToWorld& ltw, SpriteData& sprite) {
                // 非表示はスキップ
                if (!sprite.visible) {
                    return;
                }

                // テクスチャ取得
                Texture* tex = texMgr.Get(sprite.texture);
                if (!tex) {
                    return;
                }

                // 位置を取得（ワールド行列から）
                Vector2 position = ltw.GetPosition2D();

                // スケールを取得
                Vector3 scale3D = ltw.GetScale();
                Vector2 scale(scale3D.x, scale3D.y);

                // Z軸回転を取得（LocalTransformから）
                float rotationZ = 0.0f;
                auto* transform = world.GetComponent<LocalTransform>(actor);
                if (transform) {
                    rotationZ = transform->GetRotationZ();
                }

                // サイズが0の場合はテクスチャサイズを使用
                Vector2 size = sprite.size;
                if (size.x <= 0.0f || size.y <= 0.0f) {
                    size.x = static_cast<float>(tex->GetWidth());
                    size.y = static_cast<float>(tex->GetHeight());
                }

                // スケールを適用
                Vector2 finalScale(scale.x, scale.y);
                if (size.x > 0.0f && size.y > 0.0f) {
                    finalScale.x *= size.x / static_cast<float>(tex->GetWidth());
                    finalScale.y *= size.y / static_cast<float>(tex->GetHeight());
                }

                // UV座標からソース矩形を計算
                if (sprite.uvOffset != Vector2::Zero || sprite.uvSize != Vector2::One) {
                    Vector4 sourceRect(
                        sprite.uvOffset.x * static_cast<float>(tex->GetWidth()),
                        sprite.uvOffset.y * static_cast<float>(tex->GetHeight()),
                        sprite.uvSize.x * static_cast<float>(tex->GetWidth()),
                        sprite.uvSize.y * static_cast<float>(tex->GetHeight())
                    );

                    batch.Draw(
                        tex,
                        position,
                        sourceRect,
                        sprite.color,
                        rotationZ,
                        sprite.pivot,
                        finalScale,
                        sprite.flipX,
                        sprite.flipY,
                        sprite.sortingLayer,
                        sprite.orderInLayer
                    );
                } else {
                    batch.Draw(
                        tex,
                        position,
                        sprite.color,
                        rotationZ,
                        sprite.pivot,
                        finalScale,
                        sprite.flipX,
                        sprite.flipY,
                        sprite.sortingLayer,
                        sprite.orderInLayer
                    );
                }
            });

        batch.End();
    }

    int Priority() const override { return 0; }
    const char* Name() const override { return "SpriteRenderSystem"; }
};

} // namespace ECS
