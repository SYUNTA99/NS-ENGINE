/// @file IRHIShaderLibrary.h
/// @brief シェーダーライブラリとパーミュテーション管理
/// @details 複数シェーダーを含むライブラリ、バリエーション管理、
///          シェーダーマネージャー、プリコンパイル機能を提供。
/// @see 06-04-shader-library.md
#pragma once

#include "IRHIShader.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace NS { namespace RHI {
    //=========================================================================
    // RHIShaderLibraryDesc (06-04)
    //=========================================================================

    /// シェーダーライブラリ記述
    struct RHI_API RHIShaderLibraryDesc
    {
        /// ライブラリバイトコード
        RHIShaderBytecode bytecode;

        /// ライブラリ名（デバッグ用）
        const char* name = nullptr;

        /// エクスポート関数名リスト（nullptrで全てエクスポート）
        const char* const* exports = nullptr;

        /// エクスポート数
        uint32 exportCount = 0;
    };

    //=========================================================================
    // IRHIShaderLibrary (06-04)
    //=========================================================================

    /// シェーダーライブラリ
    class RHI_API IRHIShaderLibrary : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(ShaderLibrary)

        virtual ~IRHIShaderLibrary() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        virtual IRHIDevice* GetDevice() const = 0;
        virtual RHIShaderBytecode GetBytecode() const = 0;

        //=====================================================================
        // エクスポート
        //=====================================================================

        virtual uint32 GetExportCount() const = 0;
        virtual const char* GetExportName(uint32 index) const = 0;
        virtual bool HasExport(const char* name) const = 0;

        /// エクスポートからシェーダー作成
        virtual IRHIShader* CreateShaderFromExport(const char* exportName, EShaderFrequency frequency) = 0;
    };

    using RHIShaderLibraryRef = TRefCountPtr<IRHIShaderLibrary>;

    //=========================================================================
    // RHIPermutationKey (06-04)
    //=========================================================================

    /// シェーダーパーミュテーションキー
    /// 定義の組み合わせを一意に識別
    struct RHI_API RHIPermutationKey
    {
        uint64 bits = 0;

        constexpr RHIPermutationKey() = default;
        explicit constexpr RHIPermutationKey(uint64 b) : bits(b) {}

        bool operator==(const RHIPermutationKey& other) const { return bits == other.bits; }
        bool operator!=(const RHIPermutationKey& other) const { return bits != other.bits; }
        bool operator<(const RHIPermutationKey& other) const { return bits < other.bits; }

        /// ビット設定
        void SetBit(uint32 index, bool value)
        {
            if (value)
                bits |= (1ull << index);
            else
                bits &= ~(1ull << index);
        }

        /// ビット取得
        bool GetBit(uint32 index) const { return (bits & (1ull << index)) != 0; }

        /// 範囲値設定（複数ビット使用）
        void SetRange(uint32 startBit, uint32 numBits, uint32 value)
        {
            uint64 mask = ((1ull << numBits) - 1) << startBit;
            bits = (bits & ~mask) | ((static_cast<uint64>(value) << startBit) & mask);
        }

        /// 範囲値取得
        uint32 GetRange(uint32 startBit, uint32 numBits) const
        {
            uint64 mask = (1ull << numBits) - 1;
            return static_cast<uint32>((bits >> startBit) & mask);
        }

        size_t GetHash() const { return static_cast<size_t>(bits); }
    };

    /// パーミュテーションキーハッシュ
    struct RHIPermutationKeyHasher
    {
        size_t operator()(const RHIPermutationKey& key) const { return key.GetHash(); }
    };

    //=========================================================================
    // RHIPermutationDimension (06-04)
    //=========================================================================

    /// パーミュテーション定義
    struct RHI_API RHIPermutationDimension
    {
        const char* name = nullptr;
        uint32 startBit = 0;
        uint32 numBits = 1;
        const char* const* valueNames = nullptr;
        uint32 valueCount = 0;

        uint32 GetMaxValue() const { return (1u << numBits) - 1; }
    };

    //=========================================================================
    // RHIShaderPermutationSet (06-04)
    //=========================================================================

    /// シェーダーパーミュテーションセット
    class RHI_API RHIShaderPermutationSet
    {
    public:
        RHIShaderPermutationSet() = default;
        ~RHIShaderPermutationSet();

        /// 初期化
        void Initialize(const RHIPermutationDimension* dimensions, uint32 dimensionCount);

        /// シェーダー追加
        void AddPermutation(RHIPermutationKey key, IRHIShader* shader);

        /// シェーダー取得
        IRHIShader* GetPermutation(RHIPermutationKey key) const;

        /// シェーダーが存在するか
        bool HasPermutation(RHIPermutationKey key) const;

        /// 全パーミュテーション数取得
        uint32 GetPermutationCount() const;

        /// 次元数取得
        uint32 GetDimensionCount() const { return static_cast<uint32>(m_dimensions.size()); }

        /// 次元取得
        const RHIPermutationDimension& GetDimension(uint32 index) const { return m_dimensions[index]; }

        /// 名前から次元インデックス取得
        int32 FindDimensionIndex(const char* name) const;

        /// キービルダー
        class KeyBuilder
        {
        public:
            explicit KeyBuilder(const RHIShaderPermutationSet* set) : m_set(set) {}

            KeyBuilder& Set(const char* dimensionName, uint32 value);
            KeyBuilder& SetBool(const char* dimensionName, bool value);
            RHIPermutationKey Build() const { return m_key; }

        private:
            const RHIShaderPermutationSet* m_set;
            RHIPermutationKey m_key;
        };

        KeyBuilder BuildKey() const { return KeyBuilder(this); }

    private:
        std::vector<RHIPermutationDimension> m_dimensions;
        std::unordered_map<RHIPermutationKey, RHIShaderRef, RHIPermutationKeyHasher> m_permutations;
    };

    //=========================================================================
    // RHIShaderManager (06-04)
    //=========================================================================

    /// シェーダー読み込みコールバック
    using RHIShaderLoadCallback = std::function<RHIShaderBytecode(const char* path)>;

    /// シェーダーマネージャー
    class RHI_API RHIShaderManager
    {
    public:
        RHIShaderManager() = default;
        ~RHIShaderManager();

        bool Initialize(IRHIDevice* device);
        void Shutdown();

        //=====================================================================
        // シェーダー読み込み
        //=====================================================================

        /// シェーダー読み込み（キャッシュ付き）
        IRHIShader* LoadShader(const char* path, EShaderFrequency frequency, const char* entryPoint = "main");

        /// シェーダーライブラリ読み込み
        IRHIShaderLibrary* LoadShaderLibrary(const char* path);

        /// パーミュテーションセット読み込み
        RHIShaderPermutationSet* LoadPermutationSet(const char* basePath,
                                                    const RHIPermutationDimension* dimensions,
                                                    uint32 dimensionCount);

        //=====================================================================
        // キャッシュ管理
        //=====================================================================

        void SetLoadCallback(RHIShaderLoadCallback callback);
        void ClearCache();

        struct CacheStats
        {
            uint32 totalShaders = 0;
            uint32 cacheHits = 0;
            uint32 cacheMisses = 0;
            MemorySize memoryUsage = 0;
        };
        CacheStats GetCacheStats() const;

        //=====================================================================
        // ホットリロード
        //=====================================================================

        void EnableHotReload(bool enable);
        void CheckForChanges();
        uint32 ReloadChangedShaders();

        using ShaderChangedCallback = std::function<void(IRHIShader* oldShader, IRHIShader* newShader)>;
        void SetShaderChangedCallback(ShaderChangedCallback callback);

    private:
        IRHIDevice* m_device = nullptr;
        RHIShaderLoadCallback m_loadCallback;
        ShaderChangedCallback m_changedCallback;
        std::unordered_map<std::string, RHIShaderRef> m_shaderCache;
        bool m_hotReloadEnabled = false;
    };

    //=========================================================================
    // RHIShaderPrecompiler (06-04)
    //=========================================================================

    /// プリコンパイルオプション
    struct RHI_API RHIPrecompileOptions
    {
        const char* sourceDirectory = nullptr;
        const char* outputDirectory = nullptr;
        RHIShaderModel shaderModel = RHIShaderModel::SM_6_0();
        RHIShaderCompileOptions compileOptions;
        const RHIPermutationDimension* dimensions = nullptr;
        uint32 dimensionCount = 0;
        uint32 parallelJobs = 0;
        bool continueOnError = false;
    };

    /// プリコンパイル結果
    struct RHI_API RHIPrecompileResult
    {
        uint32 successCount = 0;
        uint32 failureCount = 0;
        uint32 skippedCount = 0;
        std::vector<std::string> errors;
        float totalTimeSeconds = 0.0f;
    };

    /// シェーダープリコンパイラー
    class RHI_API RHIShaderPrecompiler
    {
    public:
        RHIShaderPrecompiler() = default;

        /// プリコンパイル実行
        RHIPrecompileResult Precompile(const RHIPrecompileOptions& options);

        /// 進捗コールバック
        using ProgressCallback = std::function<void(uint32 current, uint32 total, const char* currentFile)>;
        void SetProgressCallback(ProgressCallback callback);

    private:
        ProgressCallback m_progressCallback;
    };

}} // namespace NS::RHI
