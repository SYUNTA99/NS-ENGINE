//----------------------------------------------------------------------------
//! @file   entity_tags.h
//! @brief  ECS Entity Tags - エンティティ状態タグ
//! @ref    Unity DOTS Prefab, Disabled, Simulate
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"

namespace ECS {

//============================================================================
//! @brief プレハブタグ
//!
//! エンティティがプレハブ（テンプレート）であることを示すマーカー。
//! 通常のクエリから自動的に除外される。
//!
//! @note サイズ: 0B（タグコンポーネント）
//!
//! @code
//! // プレハブの作成
//! auto prefab = world.CreateActor();
//! world.AddComponent<LocalTransform>(prefab, ...);
//! world.AddComponent<MeshData>(prefab, ...);
//! world.AddComponent<PrefabTag>(prefab);  // プレハブとしてマーク
//!
//! // インスタンス化（プレハブをコピー）
//! auto instance = world.Instantiate(prefab);
//! // instance にはPrefabTagがコピーされない
//!
//! // プレハブを含めてクエリ
//! world.Query<MeshData>()
//!     .WithOptions(QueryOptions::IncludePrefab)
//!     .ForEach([](Actor e, MeshData& mesh) { ... });
//! @endcode
//============================================================================
struct PrefabTag : public ITagComponentData {};

ECS_TAG_COMPONENT(PrefabTag);

//============================================================================
//! @brief 無効化タグ
//!
//! エンティティが一時的に無効であることを示すマーカー。
//! 通常のクエリから自動的に除外される。
//! GameObject.SetActive(false) に相当。
//!
//! @note サイズ: 0B（タグコンポーネント）
//!
//! @code
//! // エンティティの無効化
//! world.AddComponent<DisabledTag>(actor);
//!
//! // エンティティの有効化
//! world.RemoveComponent<DisabledTag>(actor);
//!
//! // 無効エンティティを含めてクエリ
//! world.Query<LocalTransform>()
//!     .WithOptions(QueryOptions::IncludeDisabled)
//!     .ForEach([](Actor e, LocalTransform& t) { ... });
//! @endcode
//============================================================================
struct DisabledTag : public ITagComponentData {};

ECS_TAG_COMPONENT(DisabledTag);

//============================================================================
//! @brief シミュレーションタグ
//!
//! エンティティがシミュレーション対象であることを示すマーカー。
//! Baking中のエンティティとランタイムエンティティを区別。
//!
//! @note サイズ: 0B（タグコンポーネント）
//!
//! @code
//! // シミュレーション対象としてマーク
//! world.AddComponent<SimulateTag>(actor);
//!
//! // 物理演算対象のみ処理
//! world.ForEach<SimulateTag, VelocityData, PhysicsMassData>(
//!     [](Actor e, auto&, VelocityData& vel, PhysicsMassData& mass) {
//!         // 物理演算
//!     });
//! @endcode
//============================================================================
struct SimulateTag : public ITagComponentData {};

ECS_TAG_COMPONENT(SimulateTag);

//============================================================================
//! @brief アクティブカメラタグ
//!
//! 現在アクティブなカメラを示すマーカー。
//! LODSystemやカリングシステムがカメラ位置を取得するために使用。
//!
//! @note サイズ: 0B（タグコンポーネント）
//!
//! @code
//! // メインカメラをアクティブに設定
//! auto camera = world.CreateActor();
//! world.AddComponent<Camera3DData>(camera, 60.0f, 16.0f/9.0f);
//! world.AddComponent<ActiveCameraTag>(camera);
//!
//! // LODSystemがアクティブカメラの位置を使用
//! world.ForEach<Camera3DData, ActiveCameraTag>(
//!     [](Actor e, Camera3DData& cam, ActiveCameraTag&) {
//!         // このカメラがアクティブ
//!     });
//! @endcode
//============================================================================
struct ActiveCameraTag : public ITagComponentData {};

ECS_TAG_COMPONENT(ActiveCameraTag);

} // namespace ECS
