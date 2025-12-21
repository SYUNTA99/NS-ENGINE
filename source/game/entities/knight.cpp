//----------------------------------------------------------------------------
//! @file   knight.cpp
//! @brief  Knight種族クラス実装
//----------------------------------------------------------------------------
#include "knight.h"
#include "group.h"
#include "game/systems/bind_system.h"
#include "game/bond/bondable_entity.h"
#include "engine/texture/texture_manager.h"
#include "engine/c_systems/collision_manager.h"
#include "engine/math/color.h"
#include "common/logging/logging.h"
#include <vector>

//----------------------------------------------------------------------------
Knight::Knight(const std::string& id)
    : Individual(id)
{
    // Knightはアニメーションなし（単一フレーム）
    animRows_ = 1;
    animCols_ = 1;
    animFrameInterval_ = 1;

    // ステータス設定（タンク型）
    maxHp_ = kDefaultHp;
    hp_ = maxHp_;
    attackDamage_ = kDefaultDamage;
    moveSpeed_ = kDefaultSpeed;
}

//----------------------------------------------------------------------------
void Knight::SetupTexture()
{
    // 白い■テクスチャを動的生成
    std::vector<uint32_t> pixels(kTextureSize * kTextureSize, 0xFFFFFFFF);
    texture_ = TextureManager::Get().Create2D(
        kTextureSize, kTextureSize,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D11_BIND_SHADER_RESOURCE,
        pixels.data(),
        kTextureSize * sizeof(uint32_t)
    );

    if (sprite_ && texture_) {
        sprite_->SetTexture(texture_.get());
        sprite_->SetSortingLayer(10);

        // 色を設定（白テクスチャに乗算）
        sprite_->SetColor(color_);

        // Pivot設定（中心）
        sprite_->SetPivot(
            static_cast<float>(kTextureSize) * 0.5f,
            static_cast<float>(kTextureSize) * 0.5f
        );

        // サイズ設定（少し大きめ）
        sprite_->SetSize(Vector2(48.0f, 48.0f));
    }
}

//----------------------------------------------------------------------------
void Knight::SetupCollider()
{
    if (!gameObject_) return;

    // Knightは少し大きめのコライダー
    collider_ = gameObject_->AddComponent<Collider2D>(Vector2(48, 48));
    collider_->SetLayer(0x04);  // Individual用レイヤー
    collider_->SetMask(0x0D);   // Individual(0x04) + Player(0x01) + Arrow(0x08)と衝突

    // 衝突コールバック：Playerとの衝突時に結びシステムを処理
    collider_->SetOnCollisionEnter([this](Collider2D* /*self*/, Collider2D* other) {
        if ((CollisionManager::Get().GetLayer(other->GetHandle()) & 0x01) == 0) return;
        if (!BindSystem::Get().IsEnabled()) return;

        Group* group = GetOwnerGroup();
        if (!group || group->IsDefeated()) return;

        BondableEntity entity = group;
        bool created = BindSystem::Get().MarkEntity(entity);
        if (created) {
            LOG_INFO("[Knight] Bond created via collision!");
        } else if (BindSystem::Get().HasMark()) {
            LOG_INFO("[Knight] Marked group: " + group->GetId());
        }
    });
}

//----------------------------------------------------------------------------
void Knight::SetColor(const Color& color)
{
    color_ = color;
    if (sprite_) {
        sprite_->SetColor(color_);
    }
}

//----------------------------------------------------------------------------
void Knight::Attack(Individual* target)
{
    if (!target || !target->IsAlive()) return;
    if (!IsAlive()) return;

    // Knightはアニメーションなし、即座にダメージ
    // TODO: 攻撃エフェクト追加

    // ダメージを与える
    target->TakeDamage(attackDamage_);

    LOG_INFO("[Knight] " + id_ + " attacks " + target->GetId() + " for " + std::to_string(attackDamage_) + " damage");
}
