# 名前空間リファレンス (UE5 / Nintendo SDK)

NS-ENGINEの名前空間設計の参考資料。

---

## UE5.7 名前空間設計

### 基本構造
```
UE::                              # ルート
├── Module                        # モジュール (Anim, Net, Slate, etc.)
│   ├── SubSystem                 # サブシステム (DataModel, Compression)
│   └── Private                   # 内部実装
└── Private::Module               # モジュール全体が内部の場合
```

### 新しい名前空間を作る基準

| 基準 | 説明 | 例 |
|------|------|-----|
| モジュール境界 | 独立したサブシステム | `UE::Anim`, `UE::Net`, `UE::Slate` |
| 機能分割 | モジュール内の論理的な分割 | `UE::Anim::DataModel`, `UE::Anim::Compression` |
| 実装隠蔽 | 公開APIではない | `UE::Animation::Private` |
| デバッグ専用 | デバッグビルドのみ | `UE::DebugDrawHelper` |

### サブ名前空間を作る基準

| 条件 | 作る | 作らない |
|------|------|----------|
| 独立したサブシステムか？ | `UE::Anim::Compression` | クラス内に配置 |
| 概念的に関連する型群か？ | `UE::Anim::DataModel` | 単一クラスならモジュール直下 |
| 実装詳細か？ | `::Private` に配置 | 公開APIはモジュール直下 |

### Privateパターン

```cpp
// 公開API（メインnamespace）
namespace UE::Anim {
    class FAnimationData { };  // 外部から使用可能
}

// 内部実装（::Private）
namespace UE::Animation::Private {
    void OnInvalidMaybeMappedAllocatorNum(...);  // 内部のみ
}

// フレンドアクセス用
namespace UE::Slate::Private {
    class STabPanelDrawer;  // 実装クラス
}

class SDockTab : public SBorder {
    friend UE::Slate::Private::STabPanelDrawer;
};
```

### Debugパターン

```cpp
// デバッグ専用ユーティリティ
namespace UE::DebugDrawHelper {
    FVector3f GetScaleAdjustedScreenLocation(...);
}

// 使用箇所
#if UE_ENABLE_DEBUG_DRAWING
    UE::DebugDrawHelper::...
#endif
```

### 実装パターン（AlgoImpl）

```cpp
// 内部実装
namespace AlgoImpl {
    template<typename RangeType, typename ValueType>
    auto FindBy(RangeType&& Range, const ValueType& Value) { ... }
}

// 公開API（委譲）
namespace Algo {
    template<typename RangeType, typename ValueType>
    auto Find(RangeType&& Range, const ValueType& Value) {
        return AlgoImpl::FindBy(Forward<RangeType>(Range), Value);
    }
}
```

### 深さの目安

| モジュール規模 | 深さ | 例 |
|----------------|------|-----|
| 小（ユーティリティ） | 1階層 | `Algo::Find()` |
| 中（機能モジュール） | 2階層 | `UE::Anim::DataModel` |
| 大（複雑なシステム） | 3階層 | `UE::Net::Private::GranularMemoryTracking` |

### クロスモジュール連携

```cpp
// 所有モジュールのnamespaceに配置
namespace UE::Net {
    struct FReplicationSystemUtil {
        static void ForEachBridge(const UWorld* World, ...);
        static UReplicationSystem* GetReplicationSystem(const AActor* Actor);
    };
}
```

### 非推奨API

```cpp
namespace ActorsReferencesUtils {
    UE_DEPRECATED(5.3, "Use GetActorReferences with params instead.")
    TArray<AActor*> GetExternalActorReferences(UObject* Root);

    // 新API（同じnamespace内）
    TArray<FActorReference> GetActorReferences(const FGetActorReferencesParams& InParams);
}
```

---

## Nintendo SDK (nn::) 名前空間設計

### 基本構造
```
nn::                              # ルート
├── module                        # モジュール (account, gfx, os, etc.)
│   ├── submodule                 # サブモジュール (fnd, viewer)
│   │   └── detail                # 内部実装
│   └── detail                    # モジュールの内部実装
```

### 新しいモジュールを作る基準

| 基準 | 説明 | 例 |
|------|------|-----|
| 独立した機能領域 | 単独で完結する機能 | `nn::account`, `nn::gfx`, `nn::audio` |
| 独自のライフサイクル | 初期化/終了が独立 | `nn::fs`, `nn::os` |
| 再利用可能な単位 | 他のモジュールから参照される | `nn::util`, `nn::mem` |

### ディレクトリ = namespace（厳格な1:1対応）

```
nn/
├── account/                      → namespace nn::account
│   ├── account_Api.h
│   ├── account_Types.h
│   └── detail/                   → namespace nn::account::detail
│       └── account_Impl.h
├── atk/                          → namespace nn::atk
│   ├── fnd/                      → namespace nn::atk::fnd
│   │   └── basis/                → namespace nn::atk::fnd::basis
│   └── viewer/                   → namespace nn::atk::viewer
│       └── detail/               → namespace nn::atk::viewer::detail
```

### detailパターン

```cpp
// 公開API
namespace nn { namespace account {
    Result GetUserCount(int* pOutCount) NN_NOEXCEPT;
}}

// 内部実装（detailに配置）
namespace nn { namespace account { namespace detail {
    class AsyncContextImpl { };  // 外部に公開しない
}}}
```

### ファイル命名規則

| パターン | 用途 | 例 |
|----------|------|-----|
| `Module_Api.h` | メインAPI関数 | `account_Api.h` |
| `Module_Types.h` | 型定義 | `account_Types.h` |
| `Module_Config.h` | コンパイル時設定 | `gfx_Config.h` |
| `Module_Result.h` | 結果コード | `account_Result.h` |
| `Module_Common.h` | 共通定義 | `gfx_Common.h` |

### プラットフォーム分岐（namespaceは変えない）

```cpp
// gfx_Config.h
#if defined(NN_BUILD_CONFIG_SPEC_NX)
    #include <nn/gfx/detail/gfx_Device-api.nvn.8.h>
#elif defined(NN_BUILD_CONFIG_OS_SUPPORTS_WIN32)
    #include <nn/gfx/detail/gfx_Device-api.gl.4.h>
#endif

// どちらも同じnamespace
namespace nn { namespace gfx { namespace detail {
    // プラットフォーム固有の実装
}}}
```

### 共有型（ルートに配置）

```cpp
// nn/nn_Common.h - 全モジュールで使用
namespace nn {
    // 基本型
}

// nn/nn_Result.h - 結果システム
namespace nn {
    class Result { };
}

// nn/nn_BitTypes.h - ビット型
namespace nn {
    typedef uint64_t Bit64;
}
```

### サブモジュール階層（ATKの例）

```
nn::atk                           # Audio Toolkit本体
├── nn::atk::fnd                  # 基盤ユーティリティ
│   ├── nn::atk::fnd::basis       # 基本型
│   ├── nn::atk::fnd::binary      # バイナリ処理
│   ├── nn::atk::fnd::io          # I/O
│   └── nn::atk::fnd::string      # 文字列
└── nn::atk::viewer               # ビューワー
    └── nn::atk::viewer::detail   # ビューワー内部実装
```

---

## 比較表

| 項目 | UE5 | Nintendo SDK |
|------|-----|--------------|
| **ルート** | `UE::` | `nn::` |
| **内部実装** | `::Private` | `::detail` |
| **デバッグ** | `::Debug`, `::DebugDrawHelper` | 別モジュール (`nn::diag`) |
| **深さ** | 2-3階層（最大5） | 2-4階層 |
| **粒度** | 粗い（モジュール単位） | 細かい（サブモジュール単位） |
| **ファイル対応** | 緩い（柔軟） | 厳格（1:1対応） |
| **命名** | PascalCase混在 | 小文字統一 |
| **非推奨** | マクロで明示 | 結果コードで処理 |

---

## namespace作成の判断フロー

### UE5方式（柔軟）

```
新機能を追加する
    │
    ├─ 独立したサブシステムか？
    │   └─ YES → UE::NewModule を作成
    │
    ├─ 既存モジュールの機能拡張か？
    │   └─ YES → UE::ExistingModule::NewFeature を作成
    │
    ├─ 実装詳細か？
    │   └─ YES → UE::Module::Private に配置
    │
    └─ 単純なユーティリティか？
        └─ YES → クラス内static or フリー関数
```

### Nintendo SDK方式（厳格）

```
新機能を追加する
    │
    ├─ 新しい機能領域か？
    │   └─ YES → nn::newmodule ディレクトリ作成
    │            namespace nn::newmodule を作成
    │
    ├─ 既存モジュールのサブ機能か？
    │   └─ YES → nn/module/submodule/ ディレクトリ作成
    │            namespace nn::module::submodule を作成
    │
    └─ 実装詳細か？
        └─ YES → nn/module/detail/ に配置
                 namespace nn::module::detail を作成
```

---

## 設計思想の違い

### UE5: 実用主義・進化重視
- 高速なイテレーション
- APIの段階的な非推奨化
- 柔軟なnamespace構成
- 後方互換性は警告で対応

### Nintendo SDK: 安定性・予測可能性重視
- API安定性を最優先
- ディレクトリ構造 = namespace（厳格）
- 結果コードによるバージョニング
- 破壊的変更を避ける設計
