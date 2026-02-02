//----------------------------------------------------------------------------
//! @file   scenes.h
//! @brief  全シーンのインクルード（正しい順序で）
//----------------------------------------------------------------------------
#pragma once

// 基底クラス
#include "engine/scene/scene.h"
#include "engine/scene/scene_manager.h"

// シーン（依存順）
#include "game_scene.h"        // ゲームシーン（ResultSceneを前方宣言）
#include "result_scene.h"      // リザルトシーン（TitleSceneを前方宣言）
#include "title_scene.h"       // タイトルシーン（GameSceneをinclude済み）
