//----------------------------------------------------------------------------
//! @file   transform_tags.h
//! @brief  ECS Transform系タグコンポーネント
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"

namespace ECS {

//============================================================================
//! @brief トランスフォーム変更タグ
//!
//! Position/Rotation/Scaleが変更されたことを示すマーカー。
//! TransformSystemがこのタグを持つエンティティのみ再計算。
//!
//! @note フレーム終了時に自動削除される。
//!       Position等を変更したらこのタグを追加。
//!
//! @code
//! // 位置変更時
//! pos.value += velocity * dt;
//! world.AddComponent<TransformDirty>(actor);  // dirty mark
//!
//! // TransformSystem
//! world.Query<TransformDirty, PositionData, LocalToWorldData>()
//!     .ForEach([](Actor e, auto&, auto& pos, auto& ltw) {
//!         ltw.value = Matrix::CreateTranslation(pos.value);
//!     });
//! @endcode
//============================================================================
struct TransformDirty : public ITagComponentData {};

// コンパイル時検証
ECS_TAG_COMPONENT(TransformDirty);

//============================================================================
//! @brief 静的トランスフォームタグ
//!
//! 移動しないエンティティのマーカー。
//! TransformSystemは初回のみ計算し、以降スキップ。
//!
//! @note 背景、建物、地形など動かないオブジェクトに付与。
//!       パフォーマンス最適化に使用。
//!
//! @code
//! // 静的オブジェクト作成
//! auto building = world.CreateActor();
//! world.AddComponent<PositionData>(building, Vector3(100, 0, 0));
//! world.AddComponent<LocalToWorldData>(building);
//! world.AddComponent<StaticTransform>(building);  // 動かない
//! @endcode
//============================================================================
struct StaticTransform : public ITagComponentData {};

// コンパイル時検証
ECS_TAG_COMPONENT(StaticTransform);

//============================================================================
//! @brief 階層ルートタグ
//!
//! 親を持たないルートエンティティのマーカー。
//! TransformSystemがルートから処理を開始するのに使用。
//!
//! @note ParentDataを持たない = HierarchyRootとも判定可能だが、
//!       タグの方がクエリ効率が良い。
//!
//! @code
//! // ルートオブジェクト作成
//! auto root = world.CreateActor();
//! world.AddComponent<PositionData>(root, Vector3::Zero);
//! world.AddComponent<LocalToWorldData>(root);
//! world.AddComponent<HierarchyRoot>(root);
//! @endcode
//============================================================================
struct HierarchyRoot : public ITagComponentData {};

// コンパイル時検証
ECS_TAG_COMPONENT(HierarchyRoot);

//============================================================================
//! @brief 初期化済みタグ
//!
//! LocalToWorldが初期計算済みであることを示すマーカー。
//! StaticTransformと組み合わせて使用。
//!
//! @code
//! // TransformSystem
//! // StaticTransform + TransformInitialized → スキップ
//! // StaticTransform + !TransformInitialized → 計算してタグ追加
//! @endcode
//============================================================================
struct TransformInitialized : public ITagComponentData {};

// コンパイル時検証
ECS_TAG_COMPONENT(TransformInitialized);

} // namespace ECS
