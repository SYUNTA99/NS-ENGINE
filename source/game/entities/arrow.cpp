//----------------------------------------------------------------------------
//! @file   arrow.cpp
//! @brief  矢クラス実装
//----------------------------------------------------------------------------
#include "arrow.h"
#include "individual.h"
#include "player.h"
#include "engine/texture/texture_manager.h"
#include "engine/c_systems/sprite_batch.h"
#include "engine/c_systems/collision_manager.h"
#include "engine/debug/debug_draw.h"
#include "engine/math/color.h"
#include "common/logging/logging.h"
#include <cmath>

//----------------------------------------------------------------------------
Arrow::Arrow(Individual* owner, Individual* target, float damage)
    : owner_(owner)
    , target_(target)
    , damage_(damage)
{
}

//----------------------------------------------------------------------------
Arrow::Arrow(Individual* owner, Player* targetPlayer, float damage)
    : owner_(owner)
    , targetPlayer_(targetPlayer)
    , damage_(damage)
{
}

//----------------------------------------------------------------------------
Arrow::~Arrow() = default;

//----------------------------------------------------------------------------
void Arrow::Initialize(const Vector2& startPos, const Vector2& targetPos)
{
    // GameObject作成
    gameObject_ = std::make_unique<GameObject>("Arrow");
    transform_ = gameObject_->AddComponent<Transform2D>(startPos);
    sprite_ = gameObject_->AddComponent<SpriteRenderer>();

    // コライダー設定（矢のサイズに合わせた小さなAABB）
    collider_ = gameObject_->AddComponent<Collider2D>(Vector2(20.0f, 10.0f));
    collider_->SetLayer(0x08);  // 矢用レイヤー
    collider_->SetMask(0x05);   // Individual(0x04) + Player(0x01)と衝突

    // 衝突コールバック設定
    collider_->SetOnCollisionEnter([this](Collider2D* /*self*/, Collider2D* other) {
        if (!isActive_) return;

        // Individual対象
        if (target_ && target_->GetCollider() == other) {
            if (target_->IsAlive()) {
                target_->TakeDamage(damage_);
                isActive_ = false;
                if (owner_) {
                    LOG_INFO("[Arrow] Hit! " + owner_->GetId() + " -> " + target_->GetId() +
                             " for " + std::to_string(damage_) + " damage");
                }
            }
            return;
        }

        // Player対象
        if (targetPlayer_ && targetPlayer_->GetCollider() == other) {
            if (targetPlayer_->IsAlive()) {
                targetPlayer_->TakeDamage(damage_);
                isActive_ = false;
                if (owner_) {
                    LOG_INFO("[Arrow] Hit! " + owner_->GetId() + " -> Player for " +
                             std::to_string(damage_) + " damage");
                }
            }
            return;
        }
    });

    // 白テクスチャを作成（矢の形）
    std::vector<uint32_t> arrowPixels(16 * 4, 0xFFFFFFFF);
    texture_ = TextureManager::Get().Create2D(
        16, 4,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D11_BIND_SHADER_RESOURCE,
        arrowPixels.data(),
        16 * sizeof(uint32_t)
    );

    if (sprite_ && texture_) {
        sprite_->SetTexture(texture_.get());
        sprite_->SetColor(Color(0.8f, 0.6f, 0.2f, 1.0f)); // 茶色
        sprite_->SetPivot(8.0f, 2.0f); // 中心
        sprite_->SetSortingLayer(15);
    }

    // 方向計算
    Vector2 diff = targetPos - startPos;
    float length = diff.Length();
    if (length > 0.0f) {
        direction_ = diff / length;
    } else {
        direction_ = Vector2(1.0f, 0.0f);
    }

    // 回転設定（矢の向き）
    if (transform_) {
        float angle = std::atan2(direction_.y, direction_.x);
        transform_->SetRotation(angle);
    }

    isActive_ = true;
    lifetime_ = 0.0f;
}

//----------------------------------------------------------------------------
void Arrow::Update(float dt)
{
    if (!isActive_) return;

    // 寿命チェック
    lifetime_ += dt;
    if (lifetime_ >= kMaxLifetime) {
        isActive_ = false;
        return;
    }

    // 移動
    if (transform_) {
        Vector2 pos = transform_->GetPosition();
        pos.x += direction_.x * speed_ * dt;
        pos.y += direction_.y * speed_ * dt;
        transform_->SetPosition(pos);
    }

    // 命中チェックはCollisionManagerのコールバックで自動処理

    // GameObject更新
    if (gameObject_) {
        gameObject_->Update(dt);
    }
}

//----------------------------------------------------------------------------
void Arrow::Render(SpriteBatch& /*spriteBatch*/)
{
    if (!isActive_) return;
    if (!transform_) return;

    // 矢をDEBUG_LINEで描画
    Vector2 pos = transform_->GetPosition();
    Vector2 endPos = pos + direction_ * 20.0f;  // 20px長さの矢
    Color arrowColor(0.8f, 0.5f, 0.2f, 1.0f);   // 茶色
    DEBUG_LINE(pos, endPos, arrowColor, 3.0f);
}

//----------------------------------------------------------------------------
Vector2 Arrow::GetPosition() const
{
    if (transform_) {
        return transform_->GetPosition();
    }
    return Vector2::Zero;
}

