# 06-02: IRHIShaderインターフェース

## 目的

シェーダーリソースのコアインターフェースを定義する。

## 参照ドキュメント

- 06-01-shader-bytecode.md (RHIShaderBytecode)
- 01-12-resource-base.md (IRHIResource)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIShader.h` (部分

## TODO

### 1. シェーダー記述構造体

```cpp
namespace NS::RHI
{
    /// シェーダー作成記述
    struct RHI_API RHIShaderDesc
    {
        /// シェーダーステージ
        EShaderFrequency frequency = EShaderFrequency::Vertex;

        /// バイトコード
        RHIShaderBytecode bytecode;

        /// エントリーポイント名
        const char* entryPoint = "main";

        /// シェーダーモデル
        RHIShaderModel shaderModel = RHIShaderModel::SM_6_0();

        /// デバッグ名
        const char* debugName = nullptr;

        //=====================================================================
        // ビルダー
        //=====================================================================

        static RHIShaderDesc Vertex(const RHIShaderBytecode& bc, const char* entry = "VSMain") {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Vertex;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Pixel(const RHIShaderBytecode& bc, const char* entry = "PSMain") {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Pixel;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Compute(const RHIShaderBytecode& bc, const char* entry = "CSMain") {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Compute;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Geometry(const RHIShaderBytecode& bc, const char* entry = "GSMain") {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Geometry;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Hull(const RHIShaderBytecode& bc, const char* entry = "HSMain") {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Hull;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Domain(const RHIShaderBytecode& bc, const char* entry = "DSMain") {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Domain;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }
    };
}
```

- [ ] RHIShaderDesc 構造体
- [ ] 各ステージ用ビルダー

### 2. IRHIShader インターフェース

```cpp
namespace NS::RHI
{
    /// シェーダーリソース
    class RHI_API IRHIShader : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Shader)

        virtual ~IRHIShader() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// シェーダーステージ取得
        virtual EShaderFrequency GetFrequency() const = 0;

        /// シェーダーモデルを取得
        virtual RHIShaderModel GetShaderModel() const = 0;

        /// エントリーポイント名取得
        virtual const char* GetEntryPoint() const = 0;

        /// シェーダーハッシュ取得
        virtual RHIShaderHash GetHash() const = 0;

        //=====================================================================
        // バイトコード
        //=====================================================================

        /// バイトコード取得
        virtual RHIShaderBytecode GetBytecode() const = 0;

        /// バイトコードサイズ取得
        MemorySize GetBytecodeSize() const {
            return GetBytecode().size;
        }

        //=====================================================================
        // ステージ判定
        //=====================================================================

        bool IsVertexShader() const { return GetFrequency() == EShaderFrequency::Vertex; }
        bool IsPixelShader() const { return GetFrequency() == EShaderFrequency::Pixel; }
        bool IsComputeShader() const { return GetFrequency() == EShaderFrequency::Compute; }
        bool IsGeometryShader() const { return GetFrequency() == EShaderFrequency::Geometry; }
        bool IsHullShader() const { return GetFrequency() == EShaderFrequency::Hull; }
        bool IsDomainShader() const { return GetFrequency() == EShaderFrequency::Domain; }
        bool IsRayTracingShader() const;

        /// グラフィックスステージか
        bool IsGraphicsShader() const {
            auto freq = GetFrequency();
            return freq == EShaderFrequency::Vertex ||
                   freq == EShaderFrequency::Pixel ||
                   freq == EShaderFrequency::Geometry ||
                   freq == EShaderFrequency::Hull ||
                   freq == EShaderFrequency::Domain;
        }
    };

    using RHIShaderRef = TRefCountPtr<IRHIShader>;
}
```

- [ ] IRHIShader インターフェース
- [ ] ステージ判定メソッド

### 3. シェーダー作成インターフェース

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // シェーダー作成
        //=====================================================================

        /// シェーダー作成
        virtual IRHIShader* CreateShader(
            const RHIShaderDesc& desc,
            const char* debugName = nullptr) = 0;

        /// バイトコードから頂点シェーダー作成
        IRHIShader* CreateVertexShader(
            const RHIShaderBytecode& bytecode,
            const char* entryPoint = "VSMain",
            const char* debugName = nullptr)
        {
            auto desc = RHIShaderDesc::Vertex(bytecode, entryPoint);
            return CreateShader(desc, debugName);
        }

        /// バイトコードからピクセルシェーダー作成
        IRHIShader* CreatePixelShader(
            const RHIShaderBytecode& bytecode,
            const char* entryPoint = "PSMain",
            const char* debugName = nullptr)
        {
            auto desc = RHIShaderDesc::Pixel(bytecode, entryPoint);
            return CreateShader(desc, debugName);
        }

        /// バイトコードからコンピュートシェーダー作成
        IRHIShader* CreateComputeShader(
            const RHIShaderBytecode& bytecode,
            const char* entryPoint = "CSMain",
            const char* debugName = nullptr)
        {
            auto desc = RHIShaderDesc::Compute(bytecode, entryPoint);
            return CreateShader(desc, debugName);
        }
    };
}
```

- [ ] CreateShader
- [ ] CreateVertexShader / CreatePixelShader / CreateComputeShader

### 4. シェーダーセット

```cpp
namespace NS::RHI
{
    /// グラフィックスシェーダーセット
    /// パイプライン作成用のシェーダー組み合わせ
    struct RHI_API RHIGraphicsShaders
    {
        IRHIShader* vertexShader = nullptr;
        IRHIShader* pixelShader = nullptr;
        IRHIShader* geometryShader = nullptr;  // オプション
        IRHIShader* hullShader = nullptr;      // テッセレーション用
        IRHIShader* domainShader = nullptr;    // テッセレーション用

        /// 最低限有効か（VS必要）
        bool IsValid() const { return vertexShader != nullptr; }

        /// テッセレーション使用か
        bool UsesTessellation() const {
            return hullShader != nullptr && domainShader != nullptr;
        }

        /// ジオメトリシェーダー使用か
        bool UsesGeometryShader() const {
            return geometryShader != nullptr;
        }

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIGraphicsShaders& SetVS(IRHIShader* vs) { vertexShader = vs; return *this; }
        RHIGraphicsShaders& SetPS(IRHIShader* ps) { pixelShader = ps; return *this; }
        RHIGraphicsShaders& SetGS(IRHIShader* gs) { geometryShader = gs; return *this; }
        RHIGraphicsShaders& SetHS(IRHIShader* hs) { hullShader = hs; return *this; }
        RHIGraphicsShaders& SetDS(IRHIShader* ds) { domainShader = ds; return *this; }

        /// VS + PS のみ
        static RHIGraphicsShaders Simple(IRHIShader* vs, IRHIShader* ps) {
            RHIGraphicsShaders shaders;
            shaders.vertexShader = vs;
            shaders.pixelShader = ps;
            return shaders;
        }
    };
}
```

- [ ] RHIGraphicsShaders 構造体
- [ ] IsValid / UsesTessellation

### 5. シェーダーキャッシュ

```cpp
namespace NS::RHI
{
    /// シェーダーキャッシュキー
    struct RHI_API RHIShaderCacheKey
    {
        RHIShaderHash sourceHash;       // ソースコープハッシュ
        RHIShaderModel shaderModel;     // シェーダーモデル
        EShaderFrequency frequency;     // シェーダーステージ
        uint32 compileOptionsHash;      // コンパイルオプションハッシュ

        bool operator==(const RHIShaderCacheKey& other) const;
        bool operator<(const RHIShaderCacheKey& other) const;
    };

    /// シェーダーキャッシュインターフェース
    class RHI_API IRHIShaderCache
    {
    public:
        virtual ~IRHIShaderCache() = default;

        /// キャッシュからシェーダー取得
        /// @return 見つかった場合のバイトコード、なければ空
        virtual RHIShaderBytecode Find(const RHIShaderCacheKey& key) = 0;

        /// キャッシュに追加
        virtual void Add(const RHIShaderCacheKey& key, const RHIShaderBytecode& bytecode) = 0;

        /// キャッシュをファイルに保存
        virtual bool SaveToFile(const char* path) = 0;

        /// ファイルからキャッシュをロード
        virtual bool LoadFromFile(const char* path) = 0;

        /// キャッシュクリア
        virtual void Clear() = 0;

        /// エントリ数取得
        virtual uint32 GetEntryCount() const = 0;

        /// キャッシュサイズ取得（バイト）
        virtual MemorySize GetCacheSize() const = 0;
    };
}
```

- [ ] RHIShaderCacheKey 構造体
- [ ] IRHIShaderCache インターフェース

## 検証方法

- [ ] シェーダー作成の正常動作
- [ ] ステージ判定の正確性
- [ ] シェーダーセットの検証
- [ ] キャッシュの動作確認
