/// @file RHIShaderReflection.h
/// @brief シェーダーリフレクション
/// @details シェーダーバイトコードからバインディング情報を取得するリフレクション機能を提供。
/// @see 06-03-shader-reflection.md
#pragma once

#include "RHIEnums.h"
#include "RHIPixelFormat.h"
#include "RHITypes.h"

#include <vector>

namespace NS { namespace RHI {
    // 前方宣言
    struct RHIShaderBytecode;
    struct RHIShaderModel;
    class IRHIShader;

    //=========================================================================
    // ERHIShaderInputType (06-03)
    //=========================================================================

    /// シェーダー入力タイプ
    enum class ERHIShaderInputType : uint8
    {
        ConstantBuffer,
        TextureSRV,
        BufferSRV,
        TextureUAV,
        BufferUAV,
        Sampler,
        ByteAddressBuffer,
        RWByteAddressBuffer,
        RootConstant,
        AccelerationStructure,
    };

    //=========================================================================
    // RHIShaderResourceBinding (06-03)
    //=========================================================================

    /// シェーダーリソースバインディング情報
    struct RHI_API RHIShaderResourceBinding
    {
        char name[64] = {};
        ERHIShaderInputType type = ERHIShaderInputType::ConstantBuffer;
        uint32 bindPoint = 0;
        uint32 bindCount = 1;
        uint32 space = 0;
        uint32 flags = 0;
        uint32 structureByteStride = 0;
        ERHIPixelFormat returnFormat = ERHIPixelFormat::Unknown;
    };

    //=========================================================================
    // RHIShaderVariable (06-03)
    //=========================================================================

    /// 定数バッファ変数情報
    struct RHI_API RHIShaderVariable
    {
        char name[64] = {};
        uint32 offset = 0;
        uint32 size = 0;
        uint32 elements = 1;
        uint32 rows = 0;
        uint32 columns = 0;
        uint32 typeFlags = 0;
    };

    //=========================================================================
    // RHIShaderConstantBuffer (06-03)
    //=========================================================================

    /// 定数バッファ情報
    struct RHI_API RHIShaderConstantBuffer
    {
        char name[64] = {};
        uint32 bindPoint = 0;
        uint32 space = 0;
        uint32 size = 0;
        std::vector<RHIShaderVariable> variables;
    };

    //=========================================================================
    // RHIShaderParameter (06-03)
    //=========================================================================

    /// シェーダー入出力パラメータ
    struct RHI_API RHIShaderParameter
    {
        char semanticName[32] = {};
        uint32 semanticIndex = 0;
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

        /// 使用マスク（1-15、各ビットがxyzw）
        uint8 mask = 0;

        /// 読み書きマスク
        uint8 readWriteMask = 0;

        /// ストリーム番号（GSの場合）
        uint8 stream = 0;
    };

    //=========================================================================
    // RHIInputSignature / RHIOutputSignature (06-03)
    //=========================================================================

    /// 入力レイアウトシグネチャ
    struct RHI_API RHIInputSignature
    {
        std::vector<RHIShaderParameter> parameters;

        /// セマンティクスでパラメータを検索
        const RHIShaderParameter* FindBySemantic(const char* semanticName, uint32 semanticIndex = 0) const;

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

    //=========================================================================
    // RHIComputeThreadGroup (06-03)
    //=========================================================================

    /// コンピュートシェーダースレッドグループ情報
    struct RHI_API RHIComputeThreadGroup
    {
        uint32 x = 1;
        uint32 y = 1;
        uint32 z = 1;
        uint32 sharedMemorySize = 0;
        uint32 numUsedRegisters = 0;

        /// 合計スレッド数
        uint32 GetTotalThreads() const { return x * y * z; }
    };

    //=========================================================================
    // IRHIShaderReflection (06-03)
    //=========================================================================

    /// シェーダーリフレクション
    class RHI_API IRHIShaderReflection
    {
    public:
        virtual ~IRHIShaderReflection() = default;

        //=====================================================================
        // 基本情報
        //=====================================================================

        virtual EShaderFrequency GetFrequency() const = 0;
        virtual RHIShaderModel GetShaderModel() const = 0;
        virtual uint32 GetInstructionCount() const = 0;
        virtual uint32 GetTempRegisterCount() const = 0;

        //=====================================================================
        // リソースバインディング
        //=====================================================================

        virtual uint32 GetResourceBindingCount() const = 0;
        virtual bool GetResourceBinding(uint32 index, RHIShaderResourceBinding& outBinding) const = 0;
        virtual bool FindResourceBinding(const char* name, RHIShaderResourceBinding& outBinding) const = 0;

        //=====================================================================
        // 定数バッファ
        //=====================================================================

        virtual uint32 GetConstantBufferCount() const = 0;
        virtual bool GetConstantBuffer(uint32 index, RHIShaderConstantBuffer& outCB) const = 0;
        virtual bool FindConstantBuffer(const char* name, RHIShaderConstantBuffer& outCB) const = 0;

        //=====================================================================
        // 入出力シグネチャ
        //=====================================================================

        virtual bool GetInputSignature(RHIInputSignature& outSignature) const = 0;
        virtual bool GetOutputSignature(RHIOutputSignature& outSignature) const = 0;

        //=====================================================================
        // コンピュートシェーダー
        //=====================================================================

        virtual bool GetThreadGroupSize(RHIComputeThreadGroup& outGroup) const = 0;

        //=====================================================================
        // 機能フラグ
        //=====================================================================

        virtual uint64 GetRequiredFeatureFlags() const = 0;

        bool UsesFeature(uint64 featureBit) const { return (GetRequiredFeatureFlags() & featureBit) != 0; }
    };

    //=========================================================================
    // リフレクション作成関数 (06-03)
    //=========================================================================

    /// シェーダーバイトコードからリフレクション取得
    RHI_API IRHIShaderReflection* CreateShaderReflection(const RHIShaderBytecode& bytecode);

    /// シェーダーリソースからリフレクション取得
    RHI_API IRHIShaderReflection* CreateShaderReflection(IRHIShader* shader);

    //=========================================================================
    // RHIBindingLayoutBuilder (06-03)
    //=========================================================================

    /// リフレクションからバインディングレイアウトを自動生成
    class RHI_API RHIBindingLayoutBuilder
    {
    public:
        RHIBindingLayoutBuilder() = default;

        /// シェーダーリフレクションを追加
        void AddShader(IRHIShaderReflection* reflection);

        /// シェーダーバイトコードから追加
        void AddShader(const RHIShaderBytecode& bytecode);

        /// バインディングをマージしてレイアウト生成
        bool Build();

        /// 生成されたリソースバインディング取得
        const std::vector<RHIShaderResourceBinding>& GetResourceBindings() const { return m_resourceBindings; }

        /// 生成された定数バッファ取得
        const std::vector<RHIShaderConstantBuffer>& GetConstantBuffers() const { return m_constantBuffers; }

        /// 必要なスペース数取得
        uint32 GetMaxRegisterSpace() const;

        /// Bindlessを推奨するか
        bool RecommendBindless(uint32 threshold = 16) const;

    private:
        std::vector<IRHIShaderReflection*> m_reflections;
        std::vector<RHIShaderResourceBinding> m_resourceBindings;
        std::vector<RHIShaderConstantBuffer> m_constantBuffers;
    };

}} // namespace NS::RHI
