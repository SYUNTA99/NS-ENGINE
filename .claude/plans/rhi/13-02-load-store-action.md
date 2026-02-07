# 13-02: ロードストアアクション

## 目的

レンダーパスのロードストアアクション列挙型を定義する。

## 参照ドキュメント

- 13-01-render-pass-desc.md (RHIRenderPassDesc)
- 16-01-resource-state.md (リソース状態

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIRenderPass.h` (部分

## TODO

### 1. ロードアクション

```cpp
namespace NS::RHI
{
    /// ロードアクション
    /// レンダーパス開始時のアタッチメント処理
    enum class ERHILoadAction : uint8
    {
        /// 前の内容をロード
        /// 既存の内容を保持
        Load,

        /// クリア
        /// 指定値でクリア
        Clear,

        /// ドントケア
        /// 前の内容は不定（最適化ヒント）
        DontCare,

        /// NoAccess
        /// このパスでは読み書きしない
        NoAccess,
    };
}
```

- [ ] ERHILoadAction 列挙型

### 2. ストアアクション

```cpp
namespace NS::RHI
{
    /// ストアアクション
    /// レンダーパス終了時のアタッチメント処理
    enum class ERHIStoreAction : uint8
    {
        /// 保存
        /// レンダリング結果を保存
        Store,

        /// ドントケア
        /// 結果は破棄（能（最適化ヒント）
        DontCare,

        /// リゾルブ
        /// MSAAをシングルサンプルにリゾルブ
        Resolve,

        /// 保存してリゾルブ
        /// MSAA結果を保存し、同時にリゾルブ
        StoreAndResolve,

        /// NoAccess
        /// このパスでは書き込まない
        NoAccess,
    };
}
```

- [ ] ERHIStoreAction 列挙型

### 3. クリア値

```cpp
namespace NS::RHI
{
    /// クリア値
    struct RHI_API RHIClearValue
    {
        union {
            /// カラークリア値（GBA）。
            float color[4];

            /// デプスステンシルクリア値
            struct {
                float depth;
                uint8 stencil;
            } depthStencil;
        };

        /// カラークリア値として初期化
        static RHIClearValue Color(float r, float g, float b, float a = 1.0f) {
            RHIClearValue cv;
            cv.color[0] = r;
            cv.color[1] = g;
            cv.color[2] = b;
            cv.color[3] = a;
            return cv;
        }

        /// デプスステンシルクリア値として初期化
        static RHIClearValue DepthStencil(float depth = 1.0f, uint8 stencil = 0) {
            RHIClearValue cv;
            cv.depthStencil.depth = depth;
            cv.depthStencil.stencil = stencil;
            return cv;
        }

        /// 黁
        static RHIClearValue Black() { return Color(0, 0, 0, 1); }

        /// 白
        static RHIClearValue White() { return Color(1, 1, 1, 1); }

        /// 透。
        static RHIClearValue Transparent() { return Color(0, 0, 0, 0); }

        /// デフォルトデプス
        static RHIClearValue DefaultDepth() { return DepthStencil(1.0f, 0); }

        /// 逆デプス（リバースZ）。
        static RHIClearValue ReverseDepth() { return DepthStencil(0.0f, 0); }
    };
}
```

- [ ] RHIClearValue 構造体
- [ ] プリセット

### 4. アクション最適化ヒント

```cpp
namespace NS::RHI
{
    /// ロードストアアクション最適化
    namespace RHILoadStoreOptimization
    {
        /// ロードアクションが最適か判定
        /// @param action 指定アクション
        /// @param willReadContent パス内の前の内容を読むか
        /// @return 最適化されたアクション
        inline ERHILoadAction OptimizeLoad(ERHILoadAction action, bool willReadContent)
        {
            // DontCareで前の内容を読む場合はLoadに変更
            if (action == ERHILoadAction::DontCare && willReadContent) {
                return ERHILoadAction::Load;
            }
            // Loadで前の内容を読まない場合はDontCareに変更（パフォーマンス向上）
            if (action == ERHILoadAction::Load && !willReadContent) {
                return ERHILoadAction::DontCare;
            }
            return action;
        }

        /// ストアアクションが最適か判定
        /// @param action 指定アクション
        /// @param willBeReadLater 後のパスで読まれるか
        /// @return 最適化されたアクション
        inline ERHIStoreAction OptimizeStore(ERHIStoreAction action, bool willBeReadLater)
        {
            // Storeで後で読まない場合はDontCareに変更
            if (action == ERHIStoreAction::Store && !willBeReadLater) {
                return ERHIStoreAction::DontCare;
            }
            // DontCareで後で読む場合はStoreに変更
            if (action == ERHIStoreAction::DontCare && willBeReadLater) {
                return ERHIStoreAction::Store;
            }
            return action;
        }

        /// タイルベースGPU向け最適化判定
        /// @return DontCareの使用でメモリ帯域を節約できるか
        inline bool CanBenefitFromDontCare(ERHILoadAction load, ERHIStoreAction store)
        {
            return load == ERHILoadAction::DontCare || store == ERHIStoreAction::DontCare;
        }
    }
}
```

- [ ] RHILoadStoreOptimization

### 5. プラットフォーム固有アクション

```cpp
namespace NS::RHI
{
    /// タイルメモリアクション（タイルベースGPU用）。
    enum class ERHITileMemoryAction : uint8
    {
        /// タイルメモリに保持
        KeepInTile,

        /// 共有メモリにフラッシュ
        FlushToShared,

        /// タイルメモリを破棄
        DiscardTile,
    };

    /// 拡張ロードストア記述（プラットフォーム拡張）。
    struct RHI_API RHIExtendedLoadStoreDesc
    {
        /// 標準ロードアクション
        ERHILoadAction loadAction = ERHILoadAction::Load;

        /// 標準ストアアクション
        ERHIStoreAction storeAction = ERHIStoreAction::Store;

        /// タイルメモリアクション（タイルベースGPU用）。
        ERHITileMemoryAction tileAction = ERHITileMemoryAction::KeepInTile;

        /// 圧縮状態を維持（DCC/CMask等）
        bool preserveCompression = true;

        /// FMASK保持（MSAA用）。
        bool preserveFMask = true;
    };
}
```

- [ ] ERHITileMemoryAction 列挙型
- [ ] RHIExtendedLoadStoreDesc 構造体

## 検証方法

- [ ] ロードストアアクションの組み合わせ
- [ ] クリア値の設定
- [ ] 最適化ヒントの動作
