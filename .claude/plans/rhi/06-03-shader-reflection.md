# 06-03: シェーダーリフレクション

## 目的

シェーダーバイトコードからバインディング情報を取得するリフレクション機能を定義する。

## 参照ドキュメント

- 06-02-shader-interface.md (IRHIShader)
- 01-06-enums-shader.md (EShaderFrequency)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIShaderReflection.h`

## TODO

### 1. リソースバインディング情報

```cpp
#pragma once

#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// シェーダー入力タイプ
    enum class ERHIShaderInputType : uint8
    {
        ConstantBuffer,     // cbuffer
        TextureSRV,         // Texture2D等
        BufferSRV,          // StructuredBuffer等
        TextureUAV,         // RWTexture2D等
        BufferUAV,          // RWStructuredBuffer等
        Sampler,            // SamplerState
        ByteAddressBuffer,  // ByteAddressBuffer
        RWByteAddressBuffer,// RWByteAddressBuffer
        RootConstant,           // インラインルート定数
        AccelerationStructure,  // RTアクセラレーション構造
    };

    /// シェーダーリソースバインディング情報
    struct RHI_API RHIShaderResourceBinding
    {
        /// 名前
        char name[64] = {};

        /// 入力タイプ
        ERHIShaderInputType type = ERHIShaderInputType::ConstantBuffer;

        /// バインドポイント（レジスタ番号）。
        uint32 bindPoint = 0;

        /// バインドカウント（配列の場合）
        uint32 bindCount = 1;

        /// レジスタスペース
        uint32 space = 0;

        /// フラグ（使用状況等）
        uint32 flags = 0;

        /// 要素サイズ（構造化バッファの場合）
        uint32 structureByteStride = 0;

        /// リターンタイプ（テクスチャの場合）
        EPixelFormat returnFormat = EPixelFormat::Unknown;
    };

    /// 定数バッファ変数情報
    struct RHI_API RHIShaderVariable
    {
        /// 変数名
        char name[64] = {};

        /// バッファ内のオフセット（バイト）
        uint32 offset = 0;

        /// サイズ（バイト）
        uint32 size = 0;

        /// 配列要素数（1 = スカラー/非配列）
        uint32 elements = 1;

        /// 行数（マトリクスの場合）
        uint32 rows = 0;

        /// 列数（マトリクスの場合）
        uint32 columns = 0;

        /// タイプフラグ
        uint32 typeFlags = 0;
    };

    /// 定数バッファ情報
    struct RHI_API RHIShaderConstantBuffer
    {
        /// バッファ合
        char name[64] = {};

        /// バインドポイント
        uint32 bindPoint = 0;

        /// レジスタスペース
        uint32 space = 0;

        /// サイズ（バイト）
        uint32 size = 0;

        /// 変数リスト
        std::vector<RHIShaderVariable> variables;
    };
}
```

- [ ] ERHIShaderInputType 列挙型
- [ ] RHIShaderResourceBinding 構造体
- [ ] RHIShaderVariable / RHIShaderConstantBuffer 構造体

### 2. 入出力シグネチャ

```cpp
namespace NS::RHI
{
    /// シェーダー入出力パラメータ
    struct RHI_API RHIShaderParameter
    {
        /// セマンデバッグ名
        char semanticName[32] = {};

        /// セマンデバッグインデックス
        uint32 semanticIndex = 0;

        /// レジスタ番号
        uint32 registerNumber = 0;

        /// システム値タイプ
        enum class SystemValue : uint8
        {
            None,
            Position,
            ClipDistance,
            CullDistance,
            RenderTargetArrayIndex,
            ViewportArrayIndex,
            VertexID,
            InstanceID,
            PrimitiveID,
            IsFrontFace,
            SampleIndex,
            Target,
            Depth,
            Coverage,
            DispatchThreadID,
            GroupID,
            GroupIndex,
            GroupThreadID,
        };
        SystemValue systemValue = SystemValue::None;

        /// コンポーネントタイプ
        enum class ComponentType : uint8
        {
            Unknown,
            UInt32,
            Int32,
            Float32,
        };
        ComponentType componentType = ComponentType::Unknown;

        /// 使用マスク（-15、各ビットがxyzw）。
        uint8 mask = 0;

        /// 読み書きマスク
        uint8 readWriteMask = 0;

        /// ストリーム番号（GSの場合）
        uint8 stream = 0;
    };

    /// 入力レイアウトシグネチャ
    struct RHI_API RHIInputSignature
    {
        std::vector<RHIShaderParameter> parameters;

        /// セマンデバッグでパラメータを検索
        const RHIShaderParameter* FindBySemantic(
            const char* semanticName,
            uint32 semanticIndex = 0) const;

        /// 全入力サイズ計算（バイト）
        uint32 CalculateTotalSize() const;
    };

    /// 出力シグネチャ
    struct RHI_API RHIOutputSignature
    {
        std::vector<RHIShaderParameter> parameters;

        /// レンダーターゲット数取得
        uint32 GetRenderTargetCount() const;

        /// デプス出力があるか
        bool HasDepthOutput() const;
    };
}
```

- [ ] RHIShaderParameter 構造体
- [ ] RHIInputSignature / RHIOutputSignature 構造体

### 3. スレッドグループ情報（コンピュートシェーダー）。

```cpp
namespace NS::RHI
{
    /// コンピュートシェーダースレッドグループ情報
    struct RHI_API RHIComputeThreadGroup
    {
        /// X方向スレッド数
        uint32 x = 1;

        /// Y方向スレッド数
        uint32 y = 1;

        /// Z方向スレッド数
        uint32 z = 1;

        /// 合計スレッド数
        uint32 GetTotalThreads() const { return x * y * z; }

        /// 共有メモリサイズ（バイト）
        uint32 sharedMemorySize = 0;

        /// 使用レジスタ数
        uint32 numUsedRegisters = 0;
    };
}
```

- [ ] RHIComputeThreadGroup 構造体

### 4. IRHIShaderReflection インターフェース

```cpp
namespace NS::RHI
{
    /// シェーダーリフレクション
    class RHI_API IRHIShaderReflection
    {
    public:
        virtual ~IRHIShaderReflection() = default;

        //=====================================================================
        // 基本情報
        //=====================================================================

        /// シェーダーステージ取得
        virtual EShaderFrequency GetFrequency() const = 0;

        /// シェーダーモデルを取得
        virtual RHIShaderModel GetShaderModel() const = 0;

        /// 命令数取得（概算）
        virtual uint32 GetInstructionCount() const = 0;

        /// 一時レジスタ数取得
        virtual uint32 GetTempRegisterCount() const = 0;

        //=====================================================================
        // リソースバインディング
        //=====================================================================

        /// リソースバインディング数取得
        virtual uint32 GetResourceBindingCount() const = 0;

        /// リソースバインディング取得
        virtual bool GetResourceBinding(
            uint32 index,
            RHIShaderResourceBinding& outBinding) const = 0;

        /// 名前でリソースバインディング検索
        virtual bool FindResourceBinding(
            const char* name,
            RHIShaderResourceBinding& outBinding) const = 0;

        //=====================================================================
        // 定数バッファ
        //=====================================================================

        /// 定数バッファ数取得
        virtual uint32 GetConstantBufferCount() const = 0;

        /// 定数バッファ情報を取得
        virtual bool GetConstantBuffer(
            uint32 index,
            RHIShaderConstantBuffer& outCB) const = 0;

        /// 名前で定数バッファ検索
        virtual bool FindConstantBuffer(
            const char* name,
            RHIShaderConstantBuffer& outCB) const = 0;

        //=====================================================================
        // 入出力シグネチャ
        //=====================================================================

        /// 入力シグネチャ取得
        virtual bool GetInputSignature(RHIInputSignature& outSignature) const = 0;

        /// 出力シグネチャ取得
        virtual bool GetOutputSignature(RHIOutputSignature& outSignature) const = 0;

        //=====================================================================
        // コンピュートシェーダー
        //=====================================================================

        /// スレッドグループ情報取得
        virtual bool GetThreadGroupSize(RHIComputeThreadGroup& outGroup) const = 0;

        //=====================================================================
        // 機能フラグ
        //=====================================================================

        /// 使用機能フラグ取得
        virtual uint64 GetRequiredFeatureFlags() const = 0;

        /// 特定機能を使用していか
        bool UsesFeature(uint64 featureBit) const {
            return (GetRequiredFeatureFlags() & featureBit) != 0;
        }
    };

    /// シェーダーからリフレクション取得
    IRHIShaderReflection* CreateShaderReflection(const RHIShaderBytecode& bytecode);
    IRHIShaderReflection* CreateShaderReflection(IRHIShader* shader);
}
```

- [ ] IRHIShaderReflection インターフェース
- [ ] CreateShaderReflection

### 5. バインディングレイアウト自動生成

```cpp
namespace NS::RHI
{
    /// リフレクションからルートシグネチャ記述を自動生成
    class RHI_API RHIBindingLayoutBuilder
    {
    public:
        RHIBindingLayoutBuilder() = default;

        /// シェーダーを追加
        void AddShader(IRHIShaderReflection* reflection);

        /// シェーダーを追加（バイトコードから）
        void AddShader(const RHIShaderBytecode& bytecode);

        /// バインディングをマージしてレイアウト生成
        /// @note 同名のリソースは同じバインドポイントを使用
        bool Build();

        /// 生成されたリソースバインディング取得
        const std::vector<RHIShaderResourceBinding>& GetResourceBindings() const {
            return m_resourceBindings;
        }

        /// 生成された定数バッファ取得
        const std::vector<RHIShaderConstantBuffer>& GetConstantBuffers() const {
            return m_constantBuffers;
        }

        /// 必要なスペース数取得
        uint32 GetMaxRegisterSpace() const;

        /// Bindlessを推奨するか（リソース数が閾値を超える場合）
        bool RecommendBindless(uint32 threshold = 16) const;

    private:
        std::vector<IRHIShaderReflection*> m_reflections;
        std::vector<RHIShaderResourceBinding> m_resourceBindings;
        std::vector<RHIShaderConstantBuffer> m_constantBuffers;
    };
}
```

- [ ] RHIBindingLayoutBuilder クラス
- [ ] AddShader / Build
- [ ] RecommendBindless

## 検証方法

- [ ] リフレクション情報の正確性
- [ ] 入出力シグネチャの一致確認
- [ ] コンピュートスレッドグループの取得
- [ ] バインディングレイアウト自動生成の動作
