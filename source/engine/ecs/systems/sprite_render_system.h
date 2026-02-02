//----------------------------------------------------------------------------
//! @file   sprite_render_system.h
//! @brief  ECS SpriteRenderSystem - スプライト描画システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform_data.h"
#include "engine/ecs/components/sprite_data.h"
#include "engine/ecs/components/animator_data.h"
#include "engine/c_systems/sprite_batch.h"
#include "engine/texture/texture_manager.h"

namespace ECS {

//============================================================================
//! @brief スプライト描画システム（クラスベース）
//!
//! TransformDataとSpriteDataを持つエンティティをSpriteBatchで描画する。
//!
//! @note 優先度0（描画システムの中で最初に実行）
//============================================================================
class SpriteRenderSystem final : public IRenderSystem {
public:
    //------------------------------------------------------------------------
    //! @brief 描画実行
    //------------------------------------------------------------------------
    void Render(World& world, [[maybe_unused]] float alpha) override {
        // AnimatorData → SpriteData UV同期
        world.ForEach<SpriteData, AnimatorData>(
            [](Actor, SpriteData& sprite, AnimatorData& anim) {
                sprite.uvOffset = anim.GetUVOffset();
                sprite.uvSize = anim.GetUVSize();
            });

        auto& batch = SpriteBatch::Get();
        auto& texMgr = TextureManager::Get();

        batch.Begin();

        // TransformDataとSpriteDataを持つ全エンティティを描画
        world.ForEach<TransformData, SpriteData>(
            [&batch, &texMgr](Actor, TransformData& transform, SpriteData& sprite) {
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
                Vector3 worldPos = transform.worldMatrix.Translation();
                Vector2 position(worldPos.x, worldPos.y);

                // スケールを取得
                Vector3 scale3D;
                Quaternion rotation;
                Vector3 translation;
                transform.worldMatrix.Decompose(scale3D, rotation, translation);
                Vector2 scale(scale3D.x, scale3D.y);

                // Z軸回転を取得
                float rotationZ = transform.GetRotationZ();

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
                    // スプライトシート使用時
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
                    // 全体表示
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
