# 01-06: シェーダー関連列挙型

## 目的

シェーダーステージ、シェーダータイプに関する列挙型を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part3.md (シェーダー)
- docs/RHI/RHI_Implementation_Guide_Part16.md (列挙型リファレンス)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIEnums.h` (部分)

## TODO

### 1. EShaderFrequency: シェーダーステージ

```cpp
namespace NS::RHI
{
    /// シェーダーステージ
    enum class EShaderFrequency : uint8
    {
        // 従来パイプライン
        Vertex,         // 頂点シェーダー
        Pixel,          // ピクセル（フラグメント）シェーダー
        Geometry,       // ジオメトリシェーダー
        Hull,           // ハルシェーダー（テッセレーション制御）
        Domain,         // ドメインシェーダー（テッセレーション評価）

        // コンピュート
        Compute,        // コンピュートシェーダー

        // メッシュシェーダー（SM6.5+）
        Mesh,           // メッシュシェーダー
        Amplification,  // 増幅シェーダー（タスクシェーダー）

        // レイトレーシング（SM6.3+）
        RayGen,         // レイ生成シェーダー
        RayMiss,        // ミスシェーダー
        RayClosestHit,  // 最近接ヒットシェーダー
        RayAnyHit,      // 任意ヒットシェーダー
        RayIntersection,// 交差シェーダー
        RayCallable,    // コーラブルシェーダー

        Count,

        // エイリアス
        Fragment = Pixel,
        Task = Amplification,
    };

    /// シェーダーステージ名取得
    inline const char* GetShaderFrequencyName(EShaderFrequency freq)
    {
        switch (freq) {
            case EShaderFrequency::Vertex:        return "Vertex";
            case EShaderFrequency::Pixel:         return "Pixel";
            case EShaderFrequency::Geometry:      return "Geometry";
            case EShaderFrequency::Hull:          return "Hull";
            case EShaderFrequency::Domain:        return "Domain";
            case EShaderFrequency::Compute:       return "Compute";
            case EShaderFrequency::Mesh:          return "Mesh";
            case EShaderFrequency::Amplification: return "Amplification";
            case EShaderFrequency::RayGen:        return "RayGen";
            case EShaderFrequency::RayMiss:       return "RayMiss";
            case EShaderFrequency::RayClosestHit: return "RayClosestHit";
            case EShaderFrequency::RayAnyHit:     return "RayAnyHit";
            case EShaderFrequency::RayIntersection: return "RayIntersection";
            case EShaderFrequency::RayCallable:   return "RayCallable";
            default:                               return "Unknown";
        }
    }
}
```

- [ ] EShaderFrequency 列挙型（全ステージ）
- [ ] 名前取得関数

### 2. シェーダーステージ判定

```cpp
namespace NS::RHI
{
    /// グラフィックスパイプラインのステージか
    inline bool IsGraphicsShaderStage(EShaderFrequency freq)
    {
        switch (freq) {
            case EShaderFrequency::Vertex:
            case EShaderFrequency::Pixel:
            case EShaderFrequency::Geometry:
            case EShaderFrequency::Hull:
            case EShaderFrequency::Domain:
                return true;
            default:
                return false;
        }
    }

    /// メッシュシェーダーパイプラインのステージか
    inline bool IsMeshShaderStage(EShaderFrequency freq)
    {
        return freq == EShaderFrequency::Mesh
            || freq == EShaderFrequency::Amplification;
    }

    /// レイトレーシングシェーダーか
    inline bool IsRayTracingShader(EShaderFrequency freq)
    {
        switch (freq) {
            case EShaderFrequency::RayGen:
            case EShaderFrequency::RayMiss:
            case EShaderFrequency::RayClosestHit:
            case EShaderFrequency::RayAnyHit:
            case EShaderFrequency::RayIntersection:
            case EShaderFrequency::RayCallable:
                return true;
            default:
                return false;
        }
    }

    /// コンピュートシェーダーか
    inline bool IsComputeShader(EShaderFrequency freq) {
        return freq == EShaderFrequency::Compute;
    }
}
```

- [ ] IsGraphicsShaderStage()
- [ ] IsMeshShaderStage()
- [ ] IsRayTracingShader()

### 3. EShaderStageFlags: シェーダーステージマスク

```cpp
namespace NS::RHI
{
    /// シェーダーステージフラグ（複数ステージ指定用）
    enum class EShaderStageFlags : uint32
    {
        None          = 0,
        Vertex        = 1 << 0,
        Pixel         = 1 << 1,
        Geometry      = 1 << 2,
        Hull          = 1 << 3,
        Domain        = 1 << 4,
        Compute       = 1 << 5,
        Mesh          = 1 << 6,
        Amplification = 1 << 7,

        // レイトレーシング
        RayGen        = 1 << 8,
        RayMiss       = 1 << 9,
        RayClosestHit = 1 << 10,
        RayAnyHit     = 1 << 11,
        RayIntersection = 1 << 12,
        RayCallable   = 1 << 13,

        // よく使う組み合わせ
        AllGraphics = Vertex | Pixel | Geometry | Hull | Domain,
        VertexPixel = Vertex | Pixel,
        AllRayTracing = RayGen | RayMiss | RayClosestHit | RayAnyHit
                      | RayIntersection | RayCallable,
        All = 0xFFFFFFFF,
    };
    RHI_ENUM_CLASS_FLAGS(EShaderStageFlags)

    /// EShaderFrequencyからフラグに変換
    inline EShaderStageFlags ShaderFrequencyToFlag(EShaderFrequency freq)
    {
        if (static_cast<uint8>(freq) < 14) {
            return static_cast<EShaderStageFlags>(1 << static_cast<uint8>(freq));
        }
        return EShaderStageFlags::None;
    }
}
```

- [ ] EShaderStageFlags ビットフラグ
- [ ] 変換関数

### 4. EShaderVisibility: シェーダー可視性

```cpp
namespace NS::RHI
{
    /// シェーダー可視性（ルートシグネチャ用）
    /// どのシェーダーステージからリソースが見えるか
    enum class EShaderVisibility : uint8
    {
        All,            // 全ステージから可視
        Vertex,         // 頂点シェーダーのみ
        Hull,           // ハルシェーダーのみ
        Domain,         // ドメインシェーダーのみ
        Geometry,       // ジオメトリシェーダーのみ
        Pixel,          // ピクセルシェーダーのみ
        Amplification,  // 増幅シェーダーのみ
        Mesh,           // メッシュシェーダーのみ
    };

    /// EShaderFrequencyからEShaderVisibilityに変換
    inline EShaderVisibility FrequencyToVisibility(EShaderFrequency freq)
    {
        switch (freq) {
            case EShaderFrequency::Vertex:        return EShaderVisibility::Vertex;
            case EShaderFrequency::Hull:          return EShaderVisibility::Hull;
            case EShaderFrequency::Domain:        return EShaderVisibility::Domain;
            case EShaderFrequency::Geometry:      return EShaderVisibility::Geometry;
            case EShaderFrequency::Pixel:         return EShaderVisibility::Pixel;
            case EShaderFrequency::Amplification: return EShaderVisibility::Amplification;
            case EShaderFrequency::Mesh:          return EShaderVisibility::Mesh;
            default:                               return EShaderVisibility::All;
        }
    }
}
```

- [ ] EShaderVisibility 列挙型
- [ ] 変換関数

### 5. EShaderModel: シェーダーモデル指定

```cpp
namespace NS::RHI
{
    /// コンパイル時のシェーダーモデル指定
    enum class EShaderModel : uint8
    {
        SM5_0,   // 5.0
        SM5_1,   // 5.1
        SM6_0,   // 6.0
        SM6_1,   // 6.1
        SM6_2,   // 6.2
        SM6_3,   // 6.3
        SM6_4,   // 6.4
        SM6_5,   // 6.5
        SM6_6,   // 6.6
        SM6_7,   // 6.7

        Default = SM6_0,
        Latest = SM6_7,
    };

    /// シェーダーモデル文字列取得（コンパイラ用）
    inline const char* GetShaderModelString(EShaderModel model)
    {
        switch (model) {
            case EShaderModel::SM5_0: return "5_0";
            case EShaderModel::SM5_1: return "5_1";
            case EShaderModel::SM6_0: return "6_0";
            case EShaderModel::SM6_1: return "6_1";
            case EShaderModel::SM6_2: return "6_2";
            case EShaderModel::SM6_3: return "6_3";
            case EShaderModel::SM6_4: return "6_4";
            case EShaderModel::SM6_5: return "6_5";
            case EShaderModel::SM6_6: return "6_6";
            case EShaderModel::SM6_7: return "6_7";
            default:                   return "6_0";
        }
    }
}
```

- [ ] EShaderModel 列挙型
- [ ] コンパイラ用文字列取得

## 検証方法

- [ ] 全シェーダーステージの網羅性
- [ ] フラグ変換の双方向一致
- [ ] D3D12/Vulkan仕様との対応確認
