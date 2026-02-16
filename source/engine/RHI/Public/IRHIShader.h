/// @file IRHIShader.h
/// @brief シェーダーリソースインターフェースとバイトコード管理
/// @details コンパイル済みシェーダーの管理、作成、キャッシュ機能を提供。
/// @see 06-01-shader-bytecode.md, 06-02-shader-interface.md
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace NS { namespace RHI {
    //=========================================================================
    // RHIShaderBytecode (06-01)
    //=========================================================================

    /// シェーダーバイトコード
    /// コンパイル済みシェーダープログラムを保持
    struct RHI_API RHIShaderBytecode
    {
        /// バイトコードデータ
        const void* data = nullptr;

        /// バイトコードサイズ（バイト）
        MemorySize size = 0;

        /// 有効か
        bool IsValid() const { return data != nullptr && size > 0; }

        /// 空のバイトコード
        static RHIShaderBytecode Empty() { return RHIShaderBytecode{}; }

        /// データから作成
        static RHIShaderBytecode FromData(const void* d, MemorySize s)
        {
            RHIShaderBytecode bc;
            bc.data = d;
            bc.size = s;
            return bc;
        }

        /// 配列から作成
        template <size_t N> static RHIShaderBytecode FromArray(const uint8 (&arr)[N]) { return FromData(arr, N); }

        /// コンテナから作成
        template <typename Container> static RHIShaderBytecode FromContainer(const Container& c)
        {
            return FromData(c.data(), static_cast<MemorySize>(c.size()));
        }
    };

    //=========================================================================
    // RHIShaderCompileError (06-01)
    //=========================================================================

    /// シェーダーコンパイルエラー情報
    struct RHI_API RHIShaderCompileError
    {
        std::string message;
        std::string filename;
        uint32 line = 0;
        uint32 column = 0;
        bool isWarning = false;
    };

    //=========================================================================
    // RHIShaderCompileResult (06-01)
    //=========================================================================

    /// シェーダーコンパイル結果
    struct RHI_API RHIShaderCompileResult
    {
        bool success = false;
        std::vector<uint8> bytecode;
        std::vector<RHIShaderCompileError> errors;
        float compileTimeMs = 0.0f;

        /// バイトコード取得
        RHIShaderBytecode GetBytecode() const
        {
            return success ? RHIShaderBytecode::FromContainer(bytecode) : RHIShaderBytecode::Empty();
        }

        /// エラーメッセージ取得（連結）
        std::string GetErrorString() const
        {
            std::string result;
            for (const auto& err : errors)
            {
                if (!err.isWarning)
                {
                    if (!result.empty())
                        result += "\n";
                    result += err.message;
                }
            }
            return result;
        }

        /// 警告があるか
        bool HasWarnings() const
        {
            for (const auto& err : errors)
            {
                if (err.isWarning)
                    return true;
            }
            return false;
        }
    };

    //=========================================================================
    // RHIShaderModel (06-01)
    //=========================================================================

    /// シェーダーモデル
    struct RHI_API RHIShaderModel
    {
        uint8 major = 6;
        uint8 minor = 0;

        constexpr RHIShaderModel() = default;
        constexpr RHIShaderModel(uint8 maj, uint8 min) : major(maj), minor(min) {}

        constexpr bool operator>=(const RHIShaderModel& other) const
        {
            return major > other.major || (major == other.major && minor >= other.minor);
        }
        constexpr bool operator<(const RHIShaderModel& other) const { return !(*this >= other); }
        constexpr bool operator==(const RHIShaderModel& other) const
        {
            return major == other.major && minor == other.minor;
        }
        constexpr bool operator!=(const RHIShaderModel& other) const { return !(*this == other); }

        /// 文字列表現取得
        const char* ToString() const;

        /// 定義済みシェーダーモデル
        static constexpr RHIShaderModel SM_5_0() { return {5, 0}; }
        static constexpr RHIShaderModel SM_5_1() { return {5, 1}; }
        static constexpr RHIShaderModel SM_6_0() { return {6, 0}; }
        static constexpr RHIShaderModel SM_6_1() { return {6, 1}; }
        static constexpr RHIShaderModel SM_6_2() { return {6, 2}; }
        static constexpr RHIShaderModel SM_6_3() { return {6, 3}; } // DXR 1.0
        static constexpr RHIShaderModel SM_6_4() { return {6, 4}; }
        static constexpr RHIShaderModel SM_6_5() { return {6, 5}; } // DXR 1.1, Mesh Shaders
        static constexpr RHIShaderModel SM_6_6() { return {6, 6}; }
        static constexpr RHIShaderModel SM_6_7() { return {6, 7}; }
    };

    /// シェーダーステージごとのHLSLターゲット名取得
    /// @param[out] outBuffer 結果格納先（16バイト以上）
    /// @param bufferSize outBufferのサイズ
    /// @return outBuffer
    inline const char* GetShaderTargetName(EShaderFrequency frequency,
                                           RHIShaderModel model,
                                           char* outBuffer,
                                           uint32 bufferSize)
    {
        static const char* prefixes[] = {
            "vs",  // Vertex
            "ps",  // Pixel
            "gs",  // Geometry
            "hs",  // Hull
            "ds",  // Domain
            "cs",  // Compute
            "ms",  // Mesh
            "as",  // Amplification
            "lib", // RayGen
            "lib", // RayMiss
            "lib", // RayClosestHit
            "lib", // RayAnyHit
            "lib", // RayIntersection
            "lib", // RayCallable
        };

        uint32 index = static_cast<uint32>(frequency);
        if (index >= sizeof(prefixes) / sizeof(prefixes[0]))
        {
            std::snprintf(outBuffer, bufferSize, "lib_%u_%u", model.major, model.minor);
        }
        else
        {
            std::snprintf(outBuffer, bufferSize, "%s_%u_%u", prefixes[index], model.major, model.minor);
        }
        return outBuffer;
    }

    //=========================================================================
    // ERHIShaderOptimization (06-01)
    //=========================================================================

    /// シェーダーコンパイル最適化レベル
    enum class ERHIShaderOptimization : uint8
    {
        None,   // 最適化なし（デバッグ用）
        Level1, // 軽量最適化
        Level2, // バランス（デフォルト）
        Level3, // 最大最適化
    };

    //=========================================================================
    // RHIShaderCompileOptions (06-01)
    //=========================================================================

    /// シェーダーコンパイルオプション
    struct RHI_API RHIShaderCompileOptions
    {
        RHIShaderModel shaderModel = RHIShaderModel::SM_6_0();
        ERHIShaderOptimization optimization = ERHIShaderOptimization::Level2;
        bool includeDebugInfo = false;
        bool warningsAsErrors = false;
        bool rowMajorMatrices = true;
        bool strictMode = false;
        bool ieeeStrictness = false;
        bool enable16BitTypes = false;

        /// プリプロセッサ定義
        std::vector<std::pair<std::string, std::string>> defines;

        /// インクルードパス
        std::vector<std::string> includePaths;

        /// 定義追加
        RHIShaderCompileOptions& Define(const char* name, const char* value = "1")
        {
            defines.emplace_back(name, value);
            return *this;
        }

        /// インクルードパス追加
        RHIShaderCompileOptions& AddIncludePath(const char* path)
        {
            includePaths.emplace_back(path);
            return *this;
        }
    };

    //=========================================================================
    // RHIShaderHash (06-01)
    //=========================================================================

    /// シェーダーハッシュ（128bit）
    /// シェーダーの一意識別に使用
    struct RHI_API RHIShaderHash
    {
        uint64 hash[2] = {0, 0};

        bool operator==(const RHIShaderHash& other) const
        {
            return hash[0] == other.hash[0] && hash[1] == other.hash[1];
        }
        bool operator!=(const RHIShaderHash& other) const { return !(*this == other); }
        bool operator<(const RHIShaderHash& other) const
        {
            return hash[0] < other.hash[0] || (hash[0] == other.hash[0] && hash[1] < other.hash[1]);
        }
        bool IsValid() const { return hash[0] != 0 || hash[1] != 0; }

        /// 文字列表現取得（16進数）
        std::string ToString() const;

        /// 文字列からパース
        static RHIShaderHash FromString(const char* str);

        /// バイトコードからハッシュ計算
        static RHIShaderHash Compute(const void* data, MemorySize size);

        static RHIShaderHash Compute(const RHIShaderBytecode& bytecode)
        {
            return Compute(bytecode.data, bytecode.size);
        }
    };

    /// RHIShaderHash用ハッシュ関数（std::unordered_map用）
    struct RHIShaderHashHasher
    {
        size_t operator()(const RHIShaderHash& h) const { return static_cast<size_t>(h.hash[0] ^ h.hash[1]); }
    };

    //=========================================================================
    // RHIShaderDesc (06-02)
    //=========================================================================

    /// シェーダー作成記述
    struct RHI_API RHIShaderDesc
    {
        EShaderFrequency frequency = EShaderFrequency::Vertex;
        RHIShaderBytecode bytecode;
        const char* entryPoint = "main";
        RHIShaderModel shaderModel = RHIShaderModel::SM_6_0();
        const char* debugName = nullptr;

        static RHIShaderDesc Vertex(const RHIShaderBytecode& bc, const char* entry = "VSMain")
        {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Vertex;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Pixel(const RHIShaderBytecode& bc, const char* entry = "PSMain")
        {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Pixel;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Compute(const RHIShaderBytecode& bc, const char* entry = "CSMain")
        {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Compute;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Geometry(const RHIShaderBytecode& bc, const char* entry = "GSMain")
        {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Geometry;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Hull(const RHIShaderBytecode& bc, const char* entry = "HSMain")
        {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Hull;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }

        static RHIShaderDesc Domain(const RHIShaderBytecode& bc, const char* entry = "DSMain")
        {
            RHIShaderDesc desc;
            desc.frequency = EShaderFrequency::Domain;
            desc.bytecode = bc;
            desc.entryPoint = entry;
            return desc;
        }
    };

    //=========================================================================
    // IRHIShader (06-02)
    //=========================================================================

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

        /// シェーダーモデル取得
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
        MemorySize GetBytecodeSize() const { return GetBytecode().size; }

        //=====================================================================
        // ステージ判定
        //=====================================================================

        bool IsVertexShader() const { return GetFrequency() == EShaderFrequency::Vertex; }
        bool IsPixelShader() const { return GetFrequency() == EShaderFrequency::Pixel; }
        bool IsComputeShader() const { return NS::RHI::IsComputeShader(GetFrequency()); }
        bool IsGeometryShader() const { return GetFrequency() == EShaderFrequency::Geometry; }
        bool IsHullShader() const { return GetFrequency() == EShaderFrequency::Hull; }
        bool IsDomainShader() const { return GetFrequency() == EShaderFrequency::Domain; }
        bool IsRayTracingShader() const { return NS::RHI::IsRayTracingShader(GetFrequency()); }

        /// グラフィックスステージか
        bool IsGraphicsShader() const { return IsGraphicsShaderStage(GetFrequency()); }
    };

    using RHIShaderRef = TRefCountPtr<IRHIShader>;

    //=========================================================================
    // RHIGraphicsShaders (06-02)
    //=========================================================================

    /// グラフィックスシェーダーセット
    /// パイプライン作成用のシェーダー組み合わせ
    struct RHI_API RHIGraphicsShaders
    {
        IRHIShader* vertexShader = nullptr;
        IRHIShader* pixelShader = nullptr;
        IRHIShader* geometryShader = nullptr;
        IRHIShader* hullShader = nullptr;
        IRHIShader* domainShader = nullptr;

        /// 最低限有効か（VS必須）
        bool IsValid() const { return vertexShader != nullptr; }

        /// テッセレーション使用か
        bool UsesTessellation() const { return hullShader != nullptr && domainShader != nullptr; }

        /// ジオメトリシェーダー使用か
        bool UsesGeometryShader() const { return geometryShader != nullptr; }

        RHIGraphicsShaders& SetVS(IRHIShader* vs)
        {
            vertexShader = vs;
            return *this;
        }
        RHIGraphicsShaders& SetPS(IRHIShader* ps)
        {
            pixelShader = ps;
            return *this;
        }
        RHIGraphicsShaders& SetGS(IRHIShader* gs)
        {
            geometryShader = gs;
            return *this;
        }
        RHIGraphicsShaders& SetHS(IRHIShader* hs)
        {
            hullShader = hs;
            return *this;
        }
        RHIGraphicsShaders& SetDS(IRHIShader* ds)
        {
            domainShader = ds;
            return *this;
        }

        /// VS + PS のみ
        static RHIGraphicsShaders Simple(IRHIShader* vs, IRHIShader* ps)
        {
            RHIGraphicsShaders shaders;
            shaders.vertexShader = vs;
            shaders.pixelShader = ps;
            return shaders;
        }
    };

    //=========================================================================
    // RHIShaderCacheKey (06-02)
    //=========================================================================

    /// シェーダーキャッシュキー
    struct RHI_API RHIShaderCacheKey
    {
        RHIShaderHash sourceHash;
        RHIShaderModel shaderModel;
        EShaderFrequency frequency = EShaderFrequency::Vertex;
        uint32 compileOptionsHash = 0;

        bool operator==(const RHIShaderCacheKey& other) const;
        bool operator<(const RHIShaderCacheKey& other) const;
    };

    //=========================================================================
    // IRHIShaderCache (06-02)
    //=========================================================================

    /// シェーダーキャッシュインターフェース
    class RHI_API IRHIShaderCache
    {
    public:
        virtual ~IRHIShaderCache() = default;

        /// キャッシュからシェーダー取得
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

}} // namespace NS::RHI
