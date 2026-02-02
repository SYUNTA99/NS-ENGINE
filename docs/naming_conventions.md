# NS-ENGINE 命名規則 (Naming Conventions)

本ドキュメントは、Unreal Engine 5.7 および Nintendo SDK (NVN) の命名規則を参考に、NS-ENGINE 独自の命名規則を定めたものです。

## 1. ファイル命名

### ルール
| 種類 | パターン | 例 |
|------|----------|-----|
| ソースファイル | `snake_case.cpp/.h` | `world.cpp`, `job_system.h` |
| テストファイル | `*_test.cpp` | `ecs_test.cpp`, `collision_test.cpp` |
| ディレクトリ | `snake_case/` | `source/engine/ecs/` |

### 参考比較
| NS-ENGINE | UE5 | Nintendo SDK |
|-----------|-----|--------------|
| `world.h` | `MassEntityManager.h` | `mem_StandardAllocator.h` |
| `snake_case` | `PascalCase` | `module_PascalCase` |

**採用理由**: シンプルさと既存コードとの一貫性を優先。

---

## 2. クラス・構造体命名

### ルール
| 種類 | パターン | 例 |
|------|----------|-----|
| 通常クラス | `PascalCase` | `World`, `GraphicsDevice`, `JobSystem` |
| インターフェース | `I` 接頭辞 | `ISystem`, `IRenderSystem`, `IComponentData` |
| ECSコンポーネント | `Data` 接尾辞 | `TransformData`, `VelocityData`, `SpriteData` |
| ECSタグ | `Tag` 接尾辞 | `PlayerTag`, `DeadTag`, `StaticTag` |
| ECSシステム | `System` 接尾辞 | `MovementSystem`, `RenderSystem` |
| マネージャー | `Manager` 接尾辞 | `TextureManager`, `SceneManager` |
| GPUリソース | `PascalCase` | `Buffer`, `Texture`, `Shader`, `BlendState` |

### 参考比較
| 概念 | NS-ENGINE | UE5 | Nintendo SDK |
|------|-----------|-----|--------------|
| ECSデータ | `TransformData` | `FMassTransformFragment` | - |
| ECSタグ | `PlayerTag` | `FMassActiveTag` | - |
| リソース | `Texture` | `FRHITexture` | `nvn::Texture` |
| 状態オブジェクト | `BlendState` | `FRHIBlendState` | `nvn::BlendState` |

**採用理由**: UE5の`F`接頭辞は採用せず、Nintendo SDK同様のシンプルなPascalCaseを維持。

---

## 3. 関数・メソッド命名

### ルール
| 種類 | パターン | 例 |
|------|----------|-----|
| インスタンスメソッド | `PascalCase` | `CreateActor()`, `AddComponent()`, `GetComponent()` |
| アクセサ（取得） | `Get` 接頭辞 | `GetPosition()`, `GetTransform()` |
| アクセサ（設定） | `Set` 接頭辞 | `SetPosition()`, `SetEnabled()` |
| 真偽判定 | `Is`/`Has`/`Can` 接頭辞 | `IsAlive()`, `HasComponent()`, `CanMove()` |
| ファクトリ/静的 | `Create`/`Make` 接頭辞 | `CreateOpaque()`, `MakeDefault()` |
| ライフサイクル | `On` 接頭辞 | `OnCreate()`, `OnUpdate()`, `OnDestroy()` |
| コールバック | `On` 接頭辞 | `OnCollisionEnter()`, `OnTriggerExit()` |

### 参考比較
| NS-ENGINE | UE5 | Nintendo SDK |
|-----------|-----|--------------|
| `CreateActor()` | `CreateEntity()` | `Create()` |
| `GetComponent<T>()` | `GetFragmentDataPtr<T>()` | `GetXxx()` |
| `IsAlive()` | `IsEntityValid()` | `IsEnabled()` |

**変更点**: 従来の `camelCase` から `PascalCase` へ統一（UE5/Nintendo SDK準拠）。

---

## 4. 変数命名

### ルール
| 種類 | パターン | 例 |
|------|----------|-----|
| プライベートメンバ | `snake_case_` (末尾アンダースコア) | `container_`, `initialized_`, `device_` |
| パブリックメンバ | `camelCase` | `position`, `rotation`, `scale` |
| ローカル変数 | `camelCase` | `actor`, `chunkIndex`, `count` |
| 関数パラメータ | `camelCase` | `actor`, `position`, `deltaTime` |
| 出力パラメータ | `out` 接頭辞 | `outChunkIndex`, `outPosition` |
| 定数 | `k` 接頭辞 + `PascalCase` または `UPPER_CASE` | `kInvalidId`, `kChunkSize`, `MAX_ENTITIES` |

### 参考比較
| 概念 | NS-ENGINE | UE5 | Nintendo SDK |
|------|-----------|-----|--------------|
| プライベートメンバ | `device_` | `Device` (no underscore) | `m_device` |
| 定数 | `kChunkSize` | `kChunkSize` | `CHUNK_SIZE` |
| 出力パラメータ | `outIndex` | `OutIndex` | `pOutIndex` |

**採用理由**: 末尾アンダースコアは読みやすく、`m_`接頭辞より簡潔。

---

## 5. 型エイリアス・テンプレート

### ルール
| 種類 | パターン | 例 |
|------|----------|-----|
| 型エイリアス | `PascalCase` | `ArchetypeId`, `ActorId`, `JobFunction` |
| スマートポインタ | `Ptr` 接尾辞 | `CancelTokenPtr`, `JobCounterPtr` |
| 参照ラッパー | `Ref` 接尾辞 | `ComponentRef<T>`, `BufferRef<T>` |
| 型トレイト | `is_xxx_v` | `is_component_v<T>`, `is_tag_v<T>` |
| コンセプト (C++20) | `C` 接頭辞 (オプション) | `ComponentType`, `CFragment` |

### 参考比較
| 概念 | NS-ENGINE | UE5 | Nintendo SDK |
|------|-----------|-----|--------------|
| リソース参照 | `TexturePtr` | `FTextureRHIRef` | `TextureHandle` |
| 型トレイト | `is_component_v` | `TIsDerivedFrom<>::Value` | - |
| コンセプト | `ComponentType` | `CFragment`, `CTag` | - |

---

## 6. 列挙型

### ルール
```cpp
// スコープ付き列挙型 (推奨)
enum class JobPriority : uint8_t {
    High = 0,
    Normal = 1,
    Low = 2,
    Count
};

// フラグ列挙型
enum class ComponentFlags : uint32_t {
    None     = 0,
    Dirty    = 1 << 0,
    Active   = 1 << 1,
    Visible  = 1 << 2,
};
```

| 種類 | パターン | 例 |
|------|----------|-----|
| 列挙型名 | `PascalCase` | `JobPriority`, `LogLevel` |
| 列挙値 | `PascalCase` | `High`, `Normal`, `Low` |
| フラグ値 | `PascalCase` | `None`, `Dirty`, `Active` |

---

## 7. マクロ命名

### ルール
| 種類 | パターン | 例 |
|------|----------|-----|
| プロジェクトマクロ | `NS_` 接頭辞 | `NS_ASSERT`, `NS_LOG`, `NS_DISALLOW_COPY` |
| ECSマクロ | `ECS_` 接頭辞 | `ECS_COMPONENT`, `ECS_TAG`, `ECS_BUFFER_ELEMENT` |
| DX11マクロ | `DX_` 接頭辞 | `DX_CHECK`, `DX_THROW_IF_FAILED` |
| 汎用マクロ | `UPPER_SNAKE_CASE` | `SINGLETON_REGISTER`, `DEBUG_ONLY` |

### 参考比較
| NS-ENGINE | UE5 | Nintendo SDK |
|-----------|-----|--------------|
| `NS_ASSERT` | `check`, `checkf` | `NN_ASSERT` |
| `NS_DISALLOW_COPY` | `UE_NONCOPYABLE` | `NN_DISALLOW_COPY` |
| `ECS_COMPONENT` | `USTRUCT` | - |

---

## 8. 名前空間

### ルール
```cpp
// トップレベル名前空間
namespace ECS { }
namespace Memory { }
namespace DX11 { }

// 詳細実装用
namespace ECS::detail { }

// 座標系ヘルパー
namespace LH { }  // Left-Hand coordinate helpers
```

| 種類 | パターン | 例 |
|------|----------|-----|
| 主要名前空間 | `PascalCase` | `ECS`, `Memory`, `DX11` |
| 詳細/内部 | `detail` | `ECS::detail` |
| ヘルパー | 略語 (必要に応じて) | `LH` (Left-Hand) |

---

## 9. ECS固有の命名規則

### コンポーネント
```cpp
// データコンポーネント (Data接尾辞)
struct TransformData : IComponentData {
    Vector3 position = Vector3::Zero;
    Quaternion rotation = Quaternion::Identity;
    Vector3 scale = Vector3::One;
};

// タグコンポーネント (Tag接尾辞、データなし)
struct PlayerTag : ITagComponentData {};
struct DeadTag : ITagComponentData {};

// バッファ要素 (Element接尾辞)
struct WaypointElement : IBufferElement {
    Vector3 position;
    float waitTime;
};
```

### システム
```cpp
// 更新システム (System接尾辞)
class MovementSystem : public ISystem {
    int Priority() const override { return 100; }
    void OnUpdate(World& world, float dt) override;
};

// 描画システム (RenderSystem接尾辞 または System接尾辞)
class SpriteRenderSystem : public IRenderSystem {
    void OnRender(World& world, float alpha) override;
};
```

### アクセスモード
```cpp
// Query用アクセス指定子
In<T>      // 読み取り専用
Out<T>     // 書き込み専用
InOut<T>   // 読み書き
Optional<T> // オプショナル（存在しない場合はnullptr）
```

---

## 10. DirectX/GPU固有の命名規則

### リソース型
```cpp
// GPU状態オブジェクト (State接尾辞)
class BlendState { };
class DepthStencilState { };
class RasterizerState { };
class SamplerState { };

// GPUリソース (そのまま)
class Buffer { };
class Texture { };
class Shader { };

// ビュー (View接尾辞)
class RenderTargetView { };
class DepthStencilView { };
class ShaderResourceView { };
```

### ファクトリメソッド
```cpp
// プリセット作成 (Create + 形容詞)
static BlendState* CreateOpaque();
static BlendState* CreateAlphaBlend();
static BlendState* CreateAdditive();

// デフォルト作成
static SamplerState* CreateDefault();
static RasterizerState* CreateDefault();
```

---

## 11. ビルダーパターン (Nintendo SDK参考)

```cpp
// ビルダークラス (Builder接尾辞)
class PrefabBuilder {
public:
    PrefabBuilder& SetDefaults();

    template<typename T, typename... Args>
    PrefabBuilder& Add(Args&&... args);

    Prefab Build();
};

// 使用例
auto prefab = world.CreatePrefab()
    .Add<TransformData>(Vector3::Zero)
    .Add<VelocityData>(Vector3::Forward)
    .Build();
```

---

## 12. ドキュメントコメント

### ヘッダファイル
```cpp
//----------------------------------------------------------------------------
//! @file   world.h
//! @brief  ECS World - アクター・コンポーネント・システムの中央管理
//----------------------------------------------------------------------------
#pragma once
```

### クラス/関数
```cpp
//------------------------------------------------------------------------
//! @brief 指定したActorにコンポーネントを追加
//! @tparam T コンポーネント型
//! @param actor 対象Actor
//! @param args コンストラクタ引数
//! @return 追加したコンポーネントへのポインタ
//! @pre actor.IsValid() == true
//------------------------------------------------------------------------
template<typename T, typename... Args>
T* AddComponent(Actor actor, Args&&... args);
```

---

## 13. 命名規則サマリーテーブル

| カテゴリ | パターン | 例 |
|----------|----------|-----|
| ファイル | `snake_case.h/.cpp` | `world.h`, `job_system.cpp` |
| クラス | `PascalCase` | `World`, `GraphicsDevice` |
| インターフェース | `I` + `PascalCase` | `ISystem`, `IComponentData` |
| 関数/メソッド | `PascalCase` | `CreateActor()`, `GetComponent()` |
| プライベートメンバ | `snake_case_` | `container_`, `device_` |
| パブリックメンバ | `camelCase` | `position`, `rotation` |
| ローカル変数 | `camelCase` | `actor`, `index` |
| 定数 | `kPascalCase` または `UPPER_CASE` | `kChunkSize`, `MAX_ENTITIES` |
| マクロ | `NS_UPPER_CASE` | `NS_ASSERT`, `ECS_COMPONENT` |
| 名前空間 | `PascalCase` | `ECS`, `Memory` |
| 列挙型 | `PascalCase` | `JobPriority::High` |
| 型エイリアス | `PascalCase` | `ActorId`, `CancelTokenPtr` |
| テンプレートパラメータ | `PascalCase` | `typename T`, `typename... Ts` |
| コンポーネント | `~Data` 接尾辞 | `TransformData`, `VelocityData` |
| タグ | `~Tag` 接尾辞 | `PlayerTag`, `DeadTag` |
| システム | `~System` 接尾辞 | `MovementSystem` |
| マネージャー | `~Manager` 接尾辞 | `TextureManager` |
| GPU状態 | `~State` 接尾辞 | `BlendState`, `SamplerState` |

---

## 14. 禁止事項

### やってはいけないこと
```cpp
// ❌ ハンガリアン記法
int iCount;
bool bEnabled;
char* pszName;

// ❌ アンダースコア接頭辞 (C++予約)
int _count;
void __init();

// ❌ 略語の乱用
int cnt;
void proc();
class Mgr;

// ❌ 型名の重複
class GraphicsDeviceClass;
struct TransformDataStruct;
```

### 推奨する代替
```cpp
// ✅ 明確な名前
int count;
bool enabled;
const char* name;

// ✅ 意味のある名前
int entityCount;
void ProcessInput();
class Manager;

// ✅ シンプルな型名
class GraphicsDevice;
struct TransformData;
```

---

## 改訂履歴

| 日付 | 変更内容 |
|------|----------|
| 2026-02-01 | 初版作成（UE5/Nintendo SDK調査に基づく） |
