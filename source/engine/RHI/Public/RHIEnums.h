/// @file RHIEnums.h
/// @brief RHIコア列挙型
/// @details バックエンド種別、機能レベル、キュータイプ、パイプラインタイプを定義。
/// @see 01-05-enums-core.md
#pragma once

#include "Common/Utility/Types.h"
#include "RHIMacros.h"

namespace NS { namespace RHI {
    //=========================================================================
    // ERHIInterfaceType: バックエンド種別
    //=========================================================================

    /// RHIバックエンド種別
    enum class ERHIInterfaceType : uint8
    {
        Hidden, // 非表示（テスト用、内部実装）
        Null,   // Null実装（ヘッドレス）
        D3D11,  // DirectX 11
        D3D12,  // DirectX 12
        Vulkan, // Vulkan
        Metal,  // Metal (macOS/iOS)

        Count
    };

    /// バックエンド名取得
    inline const char* GetRHIInterfaceTypeName(ERHIInterfaceType type)
    {
        switch (type)
        {
        case ERHIInterfaceType::Hidden:
            return "Hidden";
        case ERHIInterfaceType::Null:
            return "Null";
        case ERHIInterfaceType::D3D11:
            return "D3D11";
        case ERHIInterfaceType::D3D12:
            return "D3D12";
        case ERHIInterfaceType::Vulkan:
            return "Vulkan";
        case ERHIInterfaceType::Metal:
            return "Metal";
        default:
            return "Unknown";
        }
    }

    //=========================================================================
    // ERHIFeatureLevel: 機能レベル
    //=========================================================================

    /// シェーダーモデル / 機能レベル
    enum class ERHIFeatureLevel : uint8
    {
        SM5,   // Shader Model 5.0 (D3D11相当)
        SM6,   // Shader Model 6.0
        SM6_1, // Shader Model 6.1 (SV_Barycentrics)
        SM6_2, // Shader Model 6.2 (FP16)
        SM6_3, // Shader Model 6.3 (DXR 1.0)
        SM6_4, // Shader Model 6.4 (VRS)
        SM6_5, // Shader Model 6.5 (DXR 1.1, Mesh Shaders)
        SM6_6, // Shader Model 6.6 (Atomic64, Dynamic Resources)
        SM6_7, // Shader Model 6.7

        Count
    };

    /// 機能レベル名取得
    inline const char* GetFeatureLevelName(ERHIFeatureLevel level)
    {
        switch (level)
        {
        case ERHIFeatureLevel::SM5:
            return "SM5";
        case ERHIFeatureLevel::SM6:
            return "SM6.0";
        case ERHIFeatureLevel::SM6_1:
            return "SM6.1";
        case ERHIFeatureLevel::SM6_2:
            return "SM6.2";
        case ERHIFeatureLevel::SM6_3:
            return "SM6.3";
        case ERHIFeatureLevel::SM6_4:
            return "SM6.4";
        case ERHIFeatureLevel::SM6_5:
            return "SM6.5";
        case ERHIFeatureLevel::SM6_6:
            return "SM6.6";
        case ERHIFeatureLevel::SM6_7:
            return "SM6.7";
        default:
            return "Unknown";
        }
    }

    /// 機能レベル比較
    inline bool SupportsFeatureLevel(ERHIFeatureLevel actual, ERHIFeatureLevel required)
    {
        return static_cast<uint8>(actual) >= static_cast<uint8>(required);
    }

    //=========================================================================
    // ERHIFeatureSupport: サポート状態
    //=========================================================================

    /// 機能サポート状態
    enum class ERHIFeatureSupport : uint8
    {
        Unsupported,       // 未サポート（ハードウェア非対応）
        RuntimeDependent,  // ランタイム依存（ドライバ確認必要）
        RuntimeGuaranteed, // ランタイム保証（必ずサポート）
    };

    /// サポートしているか
    inline bool IsFeatureSupported(ERHIFeatureSupport support)
    {
        return support != ERHIFeatureSupport::Unsupported;
    }

    //=========================================================================
    // ERHIQueueType: キュータイプ
    //=========================================================================

    /// コマンドキュータイプ
    enum class ERHIQueueType : uint8
    {
        Graphics, // グラフィックス（描画 + コンピュート + コピー）
        Compute,  // 非同期コンピュート（コンピュート + コピー）
        Copy,     // コピー専用（DMA）

        Count
    };

    /// キュータイプ名取得
    inline const char* GetQueueTypeName(ERHIQueueType type)
    {
        switch (type)
        {
        case ERHIQueueType::Graphics:
            return "Graphics";
        case ERHIQueueType::Compute:
            return "Compute";
        case ERHIQueueType::Copy:
            return "Copy";
        default:
            return "Unknown";
        }
    }

    /// グラフィックス機能をサポートするか
    inline bool QueueSupportsGraphics(ERHIQueueType type)
    {
        return type == ERHIQueueType::Graphics;
    }

    /// コンピュート機能をサポートするか
    inline bool QueueSupportsCompute(ERHIQueueType type)
    {
        return type == ERHIQueueType::Graphics || type == ERHIQueueType::Compute;
    }

    /// コピー機能をサポートするか（全キューでサポート）
    inline bool QueueSupportsCopy(ERHIQueueType /*type*/)
    {
        return true;
    }

    //=========================================================================
    // ERHIPipeline: パイプラインタイプ
    //=========================================================================

    /// パイプラインタイプ
    /// コマンドコンテキストの種類を識別
    enum class ERHIPipeline : uint8
    {
        Graphics,     // グラフィックスパイプライン
        AsyncCompute, // 非同期コンピュートパイプライン

        Count
    };

    /// 対応するキュータイプ取得
    inline ERHIQueueType GetQueueTypeForPipeline(ERHIPipeline pipeline)
    {
        switch (pipeline)
        {
        case ERHIPipeline::Graphics:
            return ERHIQueueType::Graphics;
        case ERHIPipeline::AsyncCompute:
            return ERHIQueueType::Compute;
        default:
            return ERHIQueueType::Graphics;
        }
    }
    //=========================================================================
    // ERHISampleCount: マルチサンプル数
    //=========================================================================

    /// マルチサンプルカウント
    enum class ERHISampleCount : uint8
    {
        Count1 = 1,  // マルチサンプルなし
        Count2 = 2,
        Count4 = 4,
        Count8 = 8,
        Count16 = 16,
        Count32 = 32,
    };

    /// サンプル数を整数に変換
    inline uint32 GetSampleCountValue(ERHISampleCount count)
    {
        return static_cast<uint32>(count);
    }

    /// サンプル数が有効か（1より大きい）
    inline bool IsMultisampled(ERHISampleCount count)
    {
        return static_cast<uint8>(count) > 1;
    }

    //=========================================================================
    // EShaderModel: シェーダーモデル
    //=========================================================================

    /// シェーダーモデルバージョン
    enum class EShaderModel : uint8
    {
        SM5_0,
        SM5_1,
        SM6_0,
        SM6_1,
        SM6_2,
        SM6_3,
        SM6_4,
        SM6_5,
        SM6_6,
        SM6_7,

        Count,

        Default = SM6_0,
        Latest = SM6_7,
    };

    /// シェーダーモデル文字列取得（コンパイラ用）
    inline const char* GetShaderModelString(EShaderModel model)
    {
        switch (model)
        {
        case EShaderModel::SM5_0:
            return "5_0";
        case EShaderModel::SM5_1:
            return "5_1";
        case EShaderModel::SM6_0:
            return "6_0";
        case EShaderModel::SM6_1:
            return "6_1";
        case EShaderModel::SM6_2:
            return "6_2";
        case EShaderModel::SM6_3:
            return "6_3";
        case EShaderModel::SM6_4:
            return "6_4";
        case EShaderModel::SM6_5:
            return "6_5";
        case EShaderModel::SM6_6:
            return "6_6";
        case EShaderModel::SM6_7:
            return "6_7";
        default:
            return "6_0";
        }
    }

    //=========================================================================
    // EShaderFrequency: シェーダーステージ
    //=========================================================================

    /// シェーダーステージ
    enum class EShaderFrequency : uint8
    {
        // 従来パイプライン
        Vertex,        // 頂点シェーダー
        Pixel,         // ピクセル（フラグメント）シェーダー
        Geometry,      // ジオメトリシェーダー
        Hull,          // ハルシェーダー（テッセレーション制御）
        Domain,        // ドメインシェーダー（テッセレーション評価）

        // コンピュート
        Compute, // コンピュートシェーダー

        // メッシュシェーダー（SM6.5+）
        Mesh,          // メッシュシェーダー
        Amplification, // 増幅シェーダー（タスクシェーダー）

        // レイトレーシング（SM6.3+）
        RayGen,         // レイ生成シェーダー
        RayMiss,        // ミスシェーダー
        RayClosestHit,  // 最近接ヒットシェーダー
        RayAnyHit,      // 任意ヒットシェーダー
        RayIntersection, // 交差シェーダー
        RayCallable,    // コーラブルシェーダー

        Count,

        // エイリアス
        Fragment = Pixel,
        Task = Amplification,
    };

    /// シェーダーステージ名取得
    inline const char* GetShaderFrequencyName(EShaderFrequency freq)
    {
        switch (freq)
        {
        case EShaderFrequency::Vertex:
            return "Vertex";
        case EShaderFrequency::Pixel:
            return "Pixel";
        case EShaderFrequency::Geometry:
            return "Geometry";
        case EShaderFrequency::Hull:
            return "Hull";
        case EShaderFrequency::Domain:
            return "Domain";
        case EShaderFrequency::Compute:
            return "Compute";
        case EShaderFrequency::Mesh:
            return "Mesh";
        case EShaderFrequency::Amplification:
            return "Amplification";
        case EShaderFrequency::RayGen:
            return "RayGen";
        case EShaderFrequency::RayMiss:
            return "RayMiss";
        case EShaderFrequency::RayClosestHit:
            return "RayClosestHit";
        case EShaderFrequency::RayAnyHit:
            return "RayAnyHit";
        case EShaderFrequency::RayIntersection:
            return "RayIntersection";
        case EShaderFrequency::RayCallable:
            return "RayCallable";
        default:
            return "Unknown";
        }
    }

    /// グラフィックスパイプラインのステージか
    inline bool IsGraphicsShaderStage(EShaderFrequency freq)
    {
        switch (freq)
        {
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
        return freq == EShaderFrequency::Mesh || freq == EShaderFrequency::Amplification;
    }

    /// レイトレーシングシェーダーか
    inline bool IsRayTracingShader(EShaderFrequency freq)
    {
        switch (freq)
        {
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
    inline bool IsComputeShader(EShaderFrequency freq)
    {
        return freq == EShaderFrequency::Compute;
    }

    //=========================================================================
    // EShaderStageFlags: シェーダーステージマスク
    //=========================================================================

    /// シェーダーステージフラグ（複数ステージ指定用）
    enum class EShaderStageFlags : uint32
    {
        None = 0,
        Vertex = 1 << 0,
        Pixel = 1 << 1,
        Geometry = 1 << 2,
        Hull = 1 << 3,
        Domain = 1 << 4,
        Compute = 1 << 5,
        Mesh = 1 << 6,
        Amplification = 1 << 7,

        // レイトレーシング
        RayGen = 1 << 8,
        RayMiss = 1 << 9,
        RayClosestHit = 1 << 10,
        RayAnyHit = 1 << 11,
        RayIntersection = 1 << 12,
        RayCallable = 1 << 13,

        // よく使う組み合わせ
        AllGraphics = Vertex | Pixel | Geometry | Hull | Domain,
        VertexPixel = Vertex | Pixel,
        AllRayTracing =
            RayGen | RayMiss | RayClosestHit | RayAnyHit | RayIntersection | RayCallable,
        All = 0xFFFFFFFF,
    };
    RHI_ENUM_CLASS_FLAGS(EShaderStageFlags)

    /// EShaderFrequencyからフラグに変換
    inline EShaderStageFlags ShaderFrequencyToFlag(EShaderFrequency freq)
    {
        if (static_cast<uint8>(freq) < 14)
        {
            return static_cast<EShaderStageFlags>(1 << static_cast<uint8>(freq));
        }
        return EShaderStageFlags::None;
    }

    //=========================================================================
    // EShaderVisibility: シェーダー可視性
    //=========================================================================

    /// シェーダー可視性（ルートシグネチャ用）
    /// どのシェーダーステージからリソースが見えるか
    enum class EShaderVisibility : uint8
    {
        All,           // 全ステージから可視
        Vertex,        // 頂点シェーダーのみ
        Hull,          // ハルシェーダーのみ
        Domain,        // ドメインシェーダーのみ
        Geometry,      // ジオメトリシェーダーのみ
        Pixel,         // ピクセルシェーダーのみ
        Amplification, // 増幅シェーダーのみ
        Mesh,          // メッシュシェーダーのみ
    };

    /// EShaderFrequencyからEShaderVisibilityに変換
    inline EShaderVisibility FrequencyToVisibility(EShaderFrequency freq)
    {
        switch (freq)
        {
        case EShaderFrequency::Vertex:
            return EShaderVisibility::Vertex;
        case EShaderFrequency::Hull:
            return EShaderVisibility::Hull;
        case EShaderFrequency::Domain:
            return EShaderVisibility::Domain;
        case EShaderFrequency::Geometry:
            return EShaderVisibility::Geometry;
        case EShaderFrequency::Pixel:
            return EShaderVisibility::Pixel;
        case EShaderFrequency::Amplification:
            return EShaderVisibility::Amplification;
        case EShaderFrequency::Mesh:
            return EShaderVisibility::Mesh;
        default:
            return EShaderVisibility::All;
        }
    }

    //=========================================================================
    // ERHIAccess: リソースアクセス状態
    //=========================================================================

    /// リソースアクセス状態（ビットフラグ）
    /// リソースが現在どのように使用されているかを表す
    enum class ERHIAccess : uint32
    {
        Unknown = 0,

        // CPU アクセス
        CPURead = 1 << 0,
        CPUWrite = 1 << 1,

        // 頂点/インデックス
        VertexBuffer = 1 << 2,
        IndexBuffer = 1 << 3,

        // 定数バッファ
        ConstantBuffer = 1 << 4,

        // シェーダーリソースビュー
        SRVGraphics = 1 << 5,
        SRVCompute = 1 << 6,

        // アンオーダードアクセス
        UAVGraphics = 1 << 7,
        UAVCompute = 1 << 8,

        // レンダーターゲット
        RenderTarget = 1 << 9,

        // デプスステンシル
        DepthStencilRead = 1 << 10,
        DepthStencilWrite = 1 << 11,

        // コピー
        CopySource = 1 << 12,
        CopyDest = 1 << 13,

        // リゾルブ
        ResolveSource = 1 << 14,
        ResolveDest = 1 << 15,

        // その他
        Present = 1 << 16,
        IndirectArgs = 1 << 17,
        StreamOutput = 1 << 18,

        // レイトレーシング
        AccelerationStructureRead = 1 << 19,
        AccelerationStructureBuild = 1 << 20,

        // 可変レートシェーディング
        ShadingRateSource = 1 << 21,

        // 便利な組み合わせ
        SRVAll = SRVGraphics | SRVCompute,
        UAVAll = UAVGraphics | UAVCompute,
        VertexOrIndexBuffer = VertexBuffer | IndexBuffer,
        ReadOnly = SRVAll | ConstantBuffer | VertexOrIndexBuffer | CopySource | IndirectArgs
                   | DepthStencilRead,
        WriteMask = UAVAll | RenderTarget | DepthStencilWrite | CopyDest | StreamOutput,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIAccess)

    /// 書き込みアクセスを含むか
    inline bool AccessHasWrite(ERHIAccess access)
    {
        return EnumHasAnyFlags(access, ERHIAccess::WriteMask);
    }

    /// 読み取りのみか
    inline bool AccessIsReadOnly(ERHIAccess access) { return !AccessHasWrite(access); }

    /// シェーダーリソースアクセスか
    inline bool AccessIsSRV(ERHIAccess access)
    {
        return EnumHasAnyFlags(access, ERHIAccess::SRVAll);
    }

    /// UAVアクセスか
    inline bool AccessIsUAV(ERHIAccess access)
    {
        return EnumHasAnyFlags(access, ERHIAccess::UAVAll);
    }

    /// コピー操作か
    inline bool AccessIsCopy(ERHIAccess access)
    {
        return EnumHasAnyFlags(access, ERHIAccess::CopySource | ERHIAccess::CopyDest);
    }

    //=========================================================================
    // ERHIDescriptorHeapType: ディスクリプタヒープタイプ
    //=========================================================================

    /// ディスクリプタヒープタイプ
    enum class ERHIDescriptorHeapType : uint8
    {
        CBV_SRV_UAV, // 定数バッファ/シェーダーリソース/UAV
        Sampler,     // サンプラー
        RTV,         // レンダーターゲットビュー
        DSV,         // デプスステンシルビュー

        Count
    };

    /// ヒープタイプ名取得
    inline const char* GetDescriptorHeapTypeName(ERHIDescriptorHeapType type)
    {
        switch (type)
        {
        case ERHIDescriptorHeapType::CBV_SRV_UAV:
            return "CBV_SRV_UAV";
        case ERHIDescriptorHeapType::Sampler:
            return "Sampler";
        case ERHIDescriptorHeapType::RTV:
            return "RTV";
        case ERHIDescriptorHeapType::DSV:
            return "DSV";
        default:
            return "Unknown";
        }
    }

    /// GPU可視にできるヒープタイプか
    inline bool CanBeGPUVisible(ERHIDescriptorHeapType type)
    {
        return type == ERHIDescriptorHeapType::CBV_SRV_UAV
               || type == ERHIDescriptorHeapType::Sampler;
    }

    //=========================================================================
    // ERHIDescriptorType: ディスクリプタタイプ
    //=========================================================================

    /// ディスクリプタタイプ（個別）
    enum class ERHIDescriptorType : uint8
    {
        CBV,     // 定数バッファビュー
        SRV,     // シェーダーリソースビュー
        UAV,     // アンオーダードアクセスビュー
        Sampler, // サンプラー
        RTV,     // レンダーターゲットビュー
        DSV,     // デプスステンシルビュー

        Count
    };

    /// 対応するヒープタイプ取得
    inline ERHIDescriptorHeapType GetHeapTypeForDescriptor(ERHIDescriptorType type)
    {
        switch (type)
        {
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

    //=========================================================================
    // ERHIDescriptorRangeType: ディスクリプタレンジタイプ
    //=========================================================================

    /// ディスクリプタレンジタイプ（ルートシグネチャ用）
    enum class ERHIDescriptorRangeType : uint8
    {
        SRV,     // t0, t1, ...
        UAV,     // u0, u1, ...
        CBV,     // b0, b1, ...
        Sampler, // s0, s1, ...

        Count
    };

    /// HLSLレジスタプレフィックス取得
    inline char GetRegisterPrefix(ERHIDescriptorRangeType type)
    {
        switch (type)
        {
        case ERHIDescriptorRangeType::SRV:
            return 't';
        case ERHIDescriptorRangeType::UAV:
            return 'u';
        case ERHIDescriptorRangeType::CBV:
            return 'b';
        case ERHIDescriptorRangeType::Sampler:
            return 's';
        default:
            return '?';
        }
    }

    //=========================================================================
    // ERHIBufferUsage: バッファ使用フラグ
    //=========================================================================

    /// バッファ使用フラグ（ビットフラグ）
    enum class ERHIBufferUsage : uint32
    {
        None = 0,

        // 基本用途
        VertexBuffer = 1 << 0,
        IndexBuffer = 1 << 1,
        ConstantBuffer = 1 << 2,

        // シェーダーリソース
        ShaderResource = 1 << 3,
        UnorderedAccess = 1 << 4,

        // 構造化バッファ
        StructuredBuffer = 1 << 5,
        ByteAddressBuffer = 1 << 6,

        // 特殊用途
        IndirectArgs = 1 << 7,
        StreamOutput = 1 << 8,
        AccelerationStructure = 1 << 9,

        // メモリ・アクセスヒント
        CPUReadable = 1 << 10,
        CPUWritable = 1 << 11,
        Dynamic = 1 << 12,
        CopySource = 1 << 13,
        CopyDest = 1 << 14,

        // 便利な組み合わせ
        DynamicVertexBuffer = VertexBuffer | Dynamic | CPUWritable,
        DynamicIndexBuffer = IndexBuffer | Dynamic | CPUWritable,
        DynamicConstantBuffer = ConstantBuffer | Dynamic | CPUWritable,
        Default = ShaderResource,
        Staging = CPUReadable | CPUWritable | CopySource | CopyDest,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIBufferUsage)

    /// 頂点/インデックスバッファか
    inline bool IsVertexOrIndexBuffer(ERHIBufferUsage usage)
    {
        return EnumHasAnyFlags(usage,
                               ERHIBufferUsage::VertexBuffer | ERHIBufferUsage::IndexBuffer);
    }

    /// シェーダーからアクセス可能か
    inline bool IsShaderAccessible(ERHIBufferUsage usage)
    {
        return EnumHasAnyFlags(usage,
                               ERHIBufferUsage::ShaderResource | ERHIBufferUsage::UnorderedAccess
                                   | ERHIBufferUsage::ConstantBuffer);
    }

    /// CPU書き込み可能か
    inline bool IsCPUWritable(ERHIBufferUsage usage)
    {
        return EnumHasAnyFlags(usage, ERHIBufferUsage::CPUWritable);
    }

    /// CPU読み取り可能か
    inline bool IsCPUReadable(ERHIBufferUsage usage)
    {
        return EnumHasAnyFlags(usage, ERHIBufferUsage::CPUReadable);
    }

    /// 動的バッファか
    inline bool IsDynamic(ERHIBufferUsage usage)
    {
        return EnumHasAnyFlags(usage, ERHIBufferUsage::Dynamic);
    }

    /// 構造化バッファか
    inline bool IsStructured(ERHIBufferUsage usage)
    {
        return EnumHasAnyFlags(
            usage, ERHIBufferUsage::StructuredBuffer | ERHIBufferUsage::ByteAddressBuffer);
    }

    //=========================================================================
    // ERHIIndexFormat: インデックスフォーマット
    //=========================================================================

    /// インデックスバッファフォーマット
    enum class ERHIIndexFormat : uint8
    {
        UInt16, // 16ビットインデックス
        UInt32, // 32ビットインデックス
    };

    /// インデックスサイズ取得（バイト）
    inline uint32 GetIndexFormatSize(ERHIIndexFormat format)
    {
        switch (format)
        {
        case ERHIIndexFormat::UInt16:
            return 2;
        case ERHIIndexFormat::UInt32:
            return 4;
        default:
            return 4;
        }
    }

    //=========================================================================
    // ERHIMapMode: マップモード
    //=========================================================================

    /// バッファマップモード
    enum class ERHIMapMode : uint8
    {
        Read,            // 読み取り専用
        Write,           // 書き込み専用
        ReadWrite,       // 読み書き両方
        WriteDiscard,    // 書き込み（以前の内容を破棄）
        WriteNoOverwrite, // 書き込み（同期なし - 上書きしない領域）
    };

    /// 読み取りアクセスを含むか
    inline bool MapModeHasRead(ERHIMapMode mode)
    {
        return mode == ERHIMapMode::Read || mode == ERHIMapMode::ReadWrite;
    }

    /// 書き込みアクセスを含むか
    inline bool MapModeHasWrite(ERHIMapMode mode) { return mode != ERHIMapMode::Read; }

    //=========================================================================
    // ERHIBufferSRVFormat: バッファSRVフォーマット
    //=========================================================================

    /// バッファSRVフォーマット（型付きバッファ用）
    enum class ERHIBufferSRVFormat : uint8
    {
        Structured, // 構造化バッファ（フォーマットなし）
        Raw,        // バイトアドレスバッファ
        Typed,      // 型付きバッファ（要 ERHIPixelFormat指定）
    };

    //=========================================================================
    // ERHITextureUsage: テクスチャ使用フラグ
    //=========================================================================

    /// テクスチャ使用フラグ（ビットフラグ）
    enum class ERHITextureUsage : uint32
    {
        None = 0,

        // シェーダーリソース
        ShaderResource = 1 << 0,
        UnorderedAccess = 1 << 1,

        // レンダーターゲット
        RenderTarget = 1 << 2,
        DepthStencil = 1 << 3,

        // スワップチェーン/表示
        Present = 1 << 4,
        Shared = 1 << 5,

        // CPU アクセス
        CPUReadable = 1 << 6,
        CPUWritable = 1 << 7,

        // 特殊機能
        GenerateMips = 1 << 8,
        Virtual = 1 << 9,
        Streamable = 1 << 10,
        ShadingRateSource = 1 << 11,
        Memoryless = 1 << 12,
        ResolveSource = 1 << 13,
        ResolveDest = 1 << 14,

        // 便利な組み合わせ
        Default = ShaderResource,
        RenderTargetShaderResource = RenderTarget | ShaderResource,
        DepthShaderResource = DepthStencil | ShaderResource,
        UnorderedShaderResource = UnorderedAccess | ShaderResource,
        Staging = CPUReadable | CPUWritable,
    };
    RHI_ENUM_CLASS_FLAGS(ERHITextureUsage)

    //=========================================================================
    // ERHITextureDimension: テクスチャ次元
    //=========================================================================

    /// テクスチャ次元
    enum class ERHITextureDimension : uint8
    {
        Texture1D,       // 1Dテクスチャ
        Texture1DArray,  // 1D配列テクスチャ
        Texture2D,       // 2Dテクスチャ
        Texture2DArray,  // 2D配列テクスチャ
        Texture2DMS,     // 2Dマルチサンプル
        Texture2DMSArray, // 2Dマルチサンプル配列
        Texture3D,       // 3Dテクスチャ（ボリューム）
        TextureCube,     // キューブマップ
        TextureCubeArray, // キューブマップ配列
    };

    /// 次元名取得
    inline const char* GetTextureDimensionName(ERHITextureDimension dim)
    {
        switch (dim)
        {
        case ERHITextureDimension::Texture1D:
            return "1D";
        case ERHITextureDimension::Texture1DArray:
            return "1DArray";
        case ERHITextureDimension::Texture2D:
            return "2D";
        case ERHITextureDimension::Texture2DArray:
            return "2DArray";
        case ERHITextureDimension::Texture2DMS:
            return "2DMS";
        case ERHITextureDimension::Texture2DMSArray:
            return "2DMSArray";
        case ERHITextureDimension::Texture3D:
            return "3D";
        case ERHITextureDimension::TextureCube:
            return "Cube";
        case ERHITextureDimension::TextureCubeArray:
            return "CubeArray";
        default:
            return "Unknown";
        }
    }

    /// 配列テクスチャか
    inline bool IsArrayTexture(ERHITextureDimension dim)
    {
        switch (dim)
        {
        case ERHITextureDimension::Texture1DArray:
        case ERHITextureDimension::Texture2DArray:
        case ERHITextureDimension::Texture2DMSArray:
        case ERHITextureDimension::TextureCubeArray:
            return true;
        default:
            return false;
        }
    }

    /// マルチサンプルテクスチャか
    inline bool IsMultisampleTexture(ERHITextureDimension dim)
    {
        return dim == ERHITextureDimension::Texture2DMS
               || dim == ERHITextureDimension::Texture2DMSArray;
    }

    /// キューブマップか
    inline bool IsCubeTexture(ERHITextureDimension dim)
    {
        return dim == ERHITextureDimension::TextureCube
               || dim == ERHITextureDimension::TextureCubeArray;
    }

    /// 3Dテクスチャか
    inline bool Is3DTexture(ERHITextureDimension dim)
    {
        return dim == ERHITextureDimension::Texture3D;
    }

    //=========================================================================
    // ERHITextureLayout: テクスチャレイアウト
    //=========================================================================

    /// テクスチャメモリレイアウト
    enum class ERHITextureLayout : uint8
    {
        Optimal, // 最適レイアウト（GPU内部形式）
        Linear,  // 線形レイアウト（CPU読み書き可能）
        Unknown, // 不明/初期状態
    };

    /// CPUアクセス可能なレイアウトか
    inline bool IsCPUAccessibleLayout(ERHITextureLayout layout)
    {
        return layout == ERHITextureLayout::Linear;
    }

    //=========================================================================
    // ERHIComponentSwizzle / RHIComponentMapping: スウィズル
    //=========================================================================

    /// コンポーネントスウィズル
    enum class ERHIComponentSwizzle : uint8
    {
        Identity, // そのまま
        Zero,     // 0
        One,      // 1
        R,        // Rチャンネル
        G,        // Gチャンネル
        B,        // Bチャンネル
        A,        // Aチャンネル
    };

    /// テクスチャスウィズルマッピング
    struct RHIComponentMapping
    {
        ERHIComponentSwizzle r = ERHIComponentSwizzle::Identity;
        ERHIComponentSwizzle g = ERHIComponentSwizzle::Identity;
        ERHIComponentSwizzle b = ERHIComponentSwizzle::Identity;
        ERHIComponentSwizzle a = ERHIComponentSwizzle::Identity;

        /// デフォルト（Identity）
        static constexpr RHIComponentMapping Identity() { return RHIComponentMapping{}; }

        /// 全チャンネルを指定値に
        static constexpr RHIComponentMapping All(ERHIComponentSwizzle s)
        {
            return RHIComponentMapping{s, s, s, s};
        }
    };

    //=========================================================================
    // ERHIBlendFactor: ブレンドファクター
    //=========================================================================

    /// ブレンドファクター
    enum class ERHIBlendFactor : uint8
    {
        Zero,
        One,
        SrcColor,
        InvSrcColor,
        SrcAlpha,
        InvSrcAlpha,
        DstColor,
        InvDstColor,
        DstAlpha,
        InvDstAlpha,
        SrcAlphaSaturate,
        BlendFactor,    // 定数ブレンドファクター
        InvBlendFactor,
        Src1Color,      // デュアルソース
        InvSrc1Color,
        Src1Alpha,
        InvSrc1Alpha,
    };

    //=========================================================================
    // ERHIBlendOp: ブレンド演算
    //=========================================================================

    /// ブレンド演算
    enum class ERHIBlendOp : uint8
    {
        Add,         // Src + Dst
        Subtract,    // Src - Dst
        RevSubtract, // Dst - Src
        Min,         // min(Src, Dst)
        Max,         // max(Src, Dst)
    };

    //=========================================================================
    // ERHIColorWriteMask: カラー書き込みマスク
    //=========================================================================

    /// カラー書き込みマスク
    enum class ERHIColorWriteMask : uint8
    {
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,

        RGB = Red | Green | Blue,
        All = Red | Green | Blue | Alpha,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIColorWriteMask)

    //=========================================================================
    // ERHICompareFunc: 比較関数
    //=========================================================================

    /// 比較関数（デプス、ステンシル、サンプラー用）
    enum class ERHICompareFunc : uint8
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
    };

    //=========================================================================
    // ERHIStencilOp: ステンシル演算
    //=========================================================================

    /// ステンシル演算
    enum class ERHIStencilOp : uint8
    {
        Keep,
        Zero,
        Replace,
        IncrSat,
        DecrSat,
        Invert,
        IncrWrap,
        DecrWrap,
    };

    //=========================================================================
    // ERHICullMode: カリングモード
    //=========================================================================

    /// カリングモード
    enum class ERHICullMode : uint8
    {
        None,  // カリングなし
        Front, // 前面カリング
        Back,  // 背面カリング
    };

    //=========================================================================
    // ERHIFillMode: フィルモード
    //=========================================================================

    /// フィルモード
    enum class ERHIFillMode : uint8
    {
        Solid,     // ソリッド（塗りつぶし）
        Wireframe, // ワイヤーフレーム
    };

    //=========================================================================
    // ERHIPrimitiveTopology: プリミティブトポロジー
    //=========================================================================

    /// プリミティブトポロジー
    enum class ERHIPrimitiveTopology : uint8
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        LineListAdj,
        LineStripAdj,
        TriangleListAdj,
        TriangleStripAdj,
        PatchList, // テッセレーション
    };

    /// 三角形トポロジーか
    inline bool IsTriangleTopology(ERHIPrimitiveTopology topology)
    {
        switch (topology)
        {
        case ERHIPrimitiveTopology::TriangleList:
        case ERHIPrimitiveTopology::TriangleStrip:
        case ERHIPrimitiveTopology::TriangleListAdj:
        case ERHIPrimitiveTopology::TriangleStripAdj:
            return true;
        default:
            return false;
        }
    }

    //=========================================================================
    // ERHIFrontFace: 前面判定
    //=========================================================================

    /// 前面の巻き方向
    enum class ERHIFrontFace : uint8
    {
        CounterClockwise, // 反時計回りが前面
        Clockwise,        // 時計回りが前面
    };

    //=========================================================================
    // ERHILogicOp: 論理演算
    //=========================================================================

    /// 論理演算（ブレンド用）
    enum class ERHILogicOp : uint8
    {
        Clear,
        Set,
        Copy,
        CopyInverted,
        Noop,
        Invert,
        And,
        Nand,
        Or,
        Nor,
        Xor,
        Equiv,
        AndReverse,
        AndInverted,
        OrReverse,
        OrInverted,
    };

    //=========================================================================
    // ERHIPredicationOp (14-04)
    //=========================================================================

    /// プレディケーション操作
    enum class ERHIPredicationOp : uint8
    {
        EqualZero,    ///< 値がゼロなら描画をスキップ
        NotEqualZero, ///< 値が非ゼロなら描画をスキップ
    };

}} // namespace NS::RHI
