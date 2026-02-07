# 01-07: アクセス状態・ディスクリプタ列挙型

## 目的

リソースアクセス状態とディスクリプタ関連の列挙型を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part6.md (リソース状態追跡)
- docs/RHI/RHI_Implementation_Guide_Part3.md (ディスクリプタ管理)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIEnums.h` (部分)

## TODO

### 1. ERHIAccess: リソースアクセス状態

```cpp
namespace NS::RHI
{
    /// リソースアクセス状態（ビットフラグ）
    /// リソースが現在どのように使用されているかを表す
    enum class ERHIAccess : uint32
    {
        Unknown                      = 0,

        // CPU アクセス
        CPURead                      = 1 << 0,   // CPU読み取り
        CPUWrite                     = 1 << 1,   // CPU書き込み

        // 頂点/インデックス
        VertexBuffer                 = 1 << 2,   // 頂点バッファとして
        IndexBuffer                  = 1 << 3,   // インデックスバッファとして

        // 定数バッファ
        ConstantBuffer               = 1 << 4,   // 定数バッファとして

        // シェーダーリソースビュー
        SRVGraphics                  = 1 << 5,   // グラフィックスシェーダーからSRV
        SRVCompute                   = 1 << 6,   // コンピュートシェーダーからSRV

        // アンオーダードアクセス
        UAVGraphics                  = 1 << 7,   // グラフィックスシェーダーからUAV
        UAVCompute                   = 1 << 8,   // コンピュートシェーダーからUAV

        // レンダーターゲット
        RenderTarget                 = 1 << 9,   // レンダーターゲットとして

        // デプスステンシル
        DepthStencilRead             = 1 << 10,  // 深度読み取り
        DepthStencilWrite            = 1 << 11,  // 深度書き込み

        // コピー
        CopySource                   = 1 << 12,  // コピー元
        CopyDest                     = 1 << 13,  // コピー先

        // リゾルブ
        ResolveSource                = 1 << 14,  // リゾルブ元
        ResolveDest                  = 1 << 15,  // リゾルブ先

        // その他
        Present                      = 1 << 16,  // プレゼント
        IndirectArgs                 = 1 << 17,  // 間接引数バッファ
        StreamOutput                 = 1 << 18,  // ストリームアウト

        // レイトレーシング
        AccelerationStructureRead    = 1 << 19,  // AS読み取り
        AccelerationStructureBuild   = 1 << 20,  // ASビルド

        // 可変レートシェーディング
        ShadingRateSource            = 1 << 21,  // VRSソース

        //=========================================================================
        // 便利な組み合わせ
        //=========================================================================

        /// SRV全般
        SRVAll = SRVGraphics | SRVCompute,

        /// UAV全般
        UAVAll = UAVGraphics | UAVCompute,

        /// 頂点/インデックス
        VertexOrIndexBuffer = VertexBuffer | IndexBuffer,

        /// 読み取り専用
        ReadOnly = SRVAll | ConstantBuffer | VertexOrIndexBuffer
                 | CopySource | IndirectArgs | DepthStencilRead,

        /// 書き込み含む
        WriteMask = UAVAll | RenderTarget | DepthStencilWrite
                  | CopyDest | StreamOutput,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIAccess)
}
```

- [ ] ERHIAccess ビットフラグ
- [ ] 便利な組み合わせ定数

### 2. アクセス状態ユーティリティ

```cpp
namespace NS::RHI
{
    /// 書き込みアクセスを含むか
    inline bool AccessHasWrite(ERHIAccess access) {
        return EnumHasAnyFlags(access, ERHIAccess::WriteMask);
    }

    /// 読み取りのみか
    inline bool AccessIsReadOnly(ERHIAccess access) {
        return !AccessHasWrite(access);
    }

    /// シェーダーリソースアクセスか
    inline bool AccessIsSRV(ERHIAccess access) {
        return EnumHasAnyFlags(access, ERHIAccess::SRVAll);
    }

    /// UAVアクセスか
    inline bool AccessIsUAV(ERHIAccess access) {
        return EnumHasAnyFlags(access, ERHIAccess::UAVAll);
    }

    /// コピー操作か
    inline bool AccessIsCopy(ERHIAccess access) {
        return EnumHasAnyFlags(access,
            ERHIAccess::CopySource | ERHIAccess::CopyDest);
    }

    /// アクセス名取得（デバッグ用）
    const char* GetAccessName(ERHIAccess access);
}
```

- [ ] アクセス判定ユーティリティ
- [ ] デバッグ用名前取得

### 3. ERHIDescriptorHeapType: ディスクリプタヒープタイプ

```cpp
namespace NS::RHI
{
    /// ディスクリプタヒープタイプ
    enum class ERHIDescriptorHeapType : uint8
    {
        CBV_SRV_UAV,    // 定数バッファ/シェーダーリソース/UAV
        Sampler,        // サンプラー
        RTV,            // レンダーターゲットビュー
        DSV,            // デプスステンシルビュー

        Count
    };

    /// ヒープタイプ名取得
    inline const char* GetDescriptorHeapTypeName(ERHIDescriptorHeapType type)
    {
        switch (type) {
            case ERHIDescriptorHeapType::CBV_SRV_UAV: return "CBV_SRV_UAV";
            case ERHIDescriptorHeapType::Sampler:    return "Sampler";
            case ERHIDescriptorHeapType::RTV:        return "RTV";
            case ERHIDescriptorHeapType::DSV:        return "DSV";
            default:                                  return "Unknown";
        }
    }

    /// GPU可視にできるヒープタイプか
    inline bool CanBeGPUVisible(ERHIDescriptorHeapType type) {
        return type == ERHIDescriptorHeapType::CBV_SRV_UAV
            || type == ERHIDescriptorHeapType::Sampler;
    }
}
```

- [ ] ERHIDescriptorHeapType 列挙型
- [ ] GPU可視判定

### 4. ERHIDescriptorType: ディスクリプタタイプ

```cpp
namespace NS::RHI
{
    /// ディスクリプタタイプ（個別）
    enum class ERHIDescriptorType : uint8
    {
        CBV,        // 定数バッファビュー
        SRV,        // シェーダーリソースビュー
        UAV,        // アンオーダードアクセスビュー
        Sampler,    // サンプラー
        RTV,        // レンダーターゲットビュー
        DSV,        // デプスステンシルビュー

        Count
    };

    /// 対応するヒープタイプ取得
    inline ERHIDescriptorHeapType GetHeapTypeForDescriptor(ERHIDescriptorType type)
    {
        switch (type) {
            case ERHIDescriptorType::CBV:
            case ERHIDescriptorType::SRV:
            case ERHIDescriptorType::UAV:
                return ERHIDescriptorHeapType::CBV_SRV_UAV;
            case ERHIDescriptorType::Sampler:
                return ERHIDescriptorHeapType::Sampler;
            case ERHIDescriptorType::RTV:
                return ERHIDescriptorHeapType::RTV;
            case ERHIDescriptorType::DSV:
                return ERHIDescriptorHeapType::DSV;
            default:
                return ERHIDescriptorHeapType::CBV_SRV_UAV;
        }
    }
}
```

- [ ] ERHIDescriptorType 列挙型
- [ ] ヒープタイプ変換

### 5. ERHIDescriptorRangeType: ディスクリプタレンジタイプ

```cpp
namespace NS::RHI
{
    /// ディスクリプタレンジタイプ（ルートシグネチャ用）
    enum class ERHIDescriptorRangeType : uint8
    {
        SRV,        // t0, t1, ... (Texture/Buffer)
        UAV,        // u0, u1, ...
        CBV,        // b0, b1, ...
        Sampler,    // s0, s1, ...

        Count
    };

    /// HLSLレジスタプレフィックス取得
    inline char GetRegisterPrefix(ERHIDescriptorRangeType type)
    {
        switch (type) {
            case ERHIDescriptorRangeType::SRV:     return 't';
            case ERHIDescriptorRangeType::UAV:     return 'u';
            case ERHIDescriptorRangeType::CBV:     return 'b';
            case ERHIDescriptorRangeType::Sampler: return 's';
            default:                                return '?';
        }
    }
}
```

- [ ] ERHIDescriptorRangeType 列挙型
- [ ] レジスタプレフィックス取得

## 検証方法

- [ ] アクセス状態の網羅性
- [ ] ビットフラグ重複なし
- [ ] D3D12/Vulkanとの対応
