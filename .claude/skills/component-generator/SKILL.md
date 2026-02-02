---
description: "ECS ComponentData + System テンプレート生成"
---

# Component Generator

ECSコンポーネントとシステムのボイラープレートを自動生成します。

## 使用方法

`/component-generator <ComponentName>`

例: `/component-generator HealthData`

## 引数

`$ARGUMENTS` でコンポーネント名を指定（`Data`サフィックスは自動付与）

- `Health` → `HealthData` コンポーネント + `HealthSystem`
- `HealthData` → そのまま使用

## 生成ファイル

1. **ComponentData ヘッダー**: `source/engine/ecs/components/<category>/<snake_case>_data.h`
2. **System ヘッダー**: `source/engine/ecs/systems/<category>/<snake_case>_system.h`
3. **テストスタブ**: `source/tests/engine/<snake_case>_system_test.cpp`

## カテゴリ分類

コンポーネント名から自動判定:
- `Transform`, `Position`, `Rotation`, `Scale`, `Local*` → `transform/`
- `Velocity`, `Acceleration`, `Movement`, `Angular*`, `Scale*` → `movement/`
- `Sprite`, `Mesh`, `Render`, `Material`, `Light`, `LOD` → `rendering/`
- `Collider`, `Collision`, `Trigger` → `collision/`
- `Camera` → `camera/`
- `Animator`, `Animation`, `Bone`, `Skeleton` → `animation/`
- `Physics`, `Mass`, `Damping`, `Gravity` → `physics/`
- その他 → `common/`

## テンプレート

### ComponentData テンプレート

```cpp
//----------------------------------------------------------------------------
//! @file   <snake_case>_data.h
//! @brief  ECS <Name>Data - <説明>
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/component_data.h"
#include "engine/math/math_types.h"

namespace ECS {

//============================================================================
//! @brief <Name>Data（POD構造体）
//!
//! <システム名>によって処理される。
//!
//! @note メモリレイアウト: <size> bytes, <align>B境界アライン
//============================================================================
struct alignas(<align>) <Name>Data : public IComponentData {
    // TODO: フィールドを定義
    float value = 0.0f;

    <Name>Data() = default;
    explicit <Name>Data(float v) : value(v) {}
};

ECS_COMPONENT(<Name>Data);

} // namespace ECS
```

### System テンプレート

```cpp
//----------------------------------------------------------------------------
//! @file   <snake_case>_system.h
//! @brief  ECS <Name>System - <説明>
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/<category>/<snake_case>_data.h"

namespace ECS {

//============================================================================
//! @brief <Name>System（更新システム）
//!
//! 入力: <Name>Data
//! 出力: <関連コンポーネント>
//!
//! @note 優先度<priority>
//============================================================================
class <Name>System final : public ISystem {
public:
    void OnUpdate(World& world, float dt) override {
        world.ForEach<<Name>Data>(
            [dt](Actor, <Name>Data& data) {
                // TODO: 更新ロジック
                (void)dt;
                (void)data;
            });
    }

    int Priority() const override { return <priority>; }
    const char* Name() const override { return "<Name>System"; }
};

} // namespace ECS
```

### Test テンプレート

```cpp
//----------------------------------------------------------------------------
//! @file   <snake_case>_system_test.cpp
//! @brief  <Name>Systemのテスト
//----------------------------------------------------------------------------
#include <gtest/gtest.h>
#include "engine/ecs/world.h"
#include "engine/ecs/components/<category>/<snake_case>_data.h"
#include "engine/ecs/systems/<category>/<snake_case>_system.h"

namespace {

//============================================================================
// <Name>System テスト
//============================================================================
TEST(<Name>SystemTest, BasicUpdate)
{
    ECS::World world;
    world.RegisterSystem<ECS::<Name>System>();

    auto actor = world.CreateActor();
    world.AddComponent<ECS::<Name>Data>(actor, 1.0f);

    world.FixedUpdate(0.016f);

    // TODO: アサーションを追加
    auto* data = world.GetComponent<ECS::<Name>Data>(actor);
    ASSERT_NE(data, nullptr);
}

} // namespace
```

## 実行手順

1. 引数からコンポーネント名を解析
2. カテゴリを自動判定
3. 既存ファイルがあれば警告して確認
4. 3ファイルを生成
5. 生成ファイル一覧を表示

## 設計規約（CLAUDE.md準拠）

- **ComponentData**: データのみ、ロジックなし（POD構造体）
- **System**: 単一責任、ForEachで処理
- **アライメント**: 16B境界推奨（Vector3含む場合）
- **命名**: `<Name>Data` / `<Name>System`
- **ECS_COMPONENT マクロ**: 型登録必須
