/// @file IRHIShaderLibrary.cpp
/// @brief シェーダーライブラリ・パーミュテーション・マネージャー実装
#include "IRHIShaderLibrary.h"
#include "IRHIDevice.h"
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIShaderPermutationSet
    //=========================================================================

    RHIShaderPermutationSet::~RHIShaderPermutationSet() = default;

    void RHIShaderPermutationSet::Initialize(const RHIPermutationDimension* dimensions, uint32 dimensionCount)
    {
        m_dimensions.assign(dimensions, dimensions + dimensionCount);
    }

    void RHIShaderPermutationSet::AddPermutation(RHIPermutationKey key, IRHIShader* shader)
    {
        m_permutations[key] = shader;
    }

    IRHIShader* RHIShaderPermutationSet::GetPermutation(RHIPermutationKey key) const
    {
        auto it = m_permutations.find(key);
        return (it != m_permutations.end()) ? it->second.Get() : nullptr;
    }

    bool RHIShaderPermutationSet::HasPermutation(RHIPermutationKey key) const
    {
        return m_permutations.find(key) != m_permutations.end();
    }

    uint32 RHIShaderPermutationSet::GetPermutationCount() const
    {
        return static_cast<uint32>(m_permutations.size());
    }

    int32 RHIShaderPermutationSet::FindDimensionIndex(const char* name) const
    {
        if (name == nullptr) {
            return -1;
}

        for (uint32 i = 0; i < static_cast<uint32>(m_dimensions.size()); ++i)
        {
            if ((m_dimensions[i].name != nullptr) && std::strcmp(m_dimensions[i].name, name) == 0) {
                return static_cast<int32>(i);
}
        }
        return -1;
    }

    //=========================================================================
    // RHIShaderPermutationSet::KeyBuilder
    //=========================================================================

    RHIShaderPermutationSet::KeyBuilder& RHIShaderPermutationSet::KeyBuilder::Set(const char* dimensionName,
                                                                                  uint32 value)
    {
        int32 const index = m_set->FindDimensionIndex(dimensionName);
        if (index >= 0)
        {
            const auto& dim = m_set->GetDimension(static_cast<uint32>(index));
            m_key.SetRange(dim.startBit, dim.numBits, value);
        }
        return *this;
    }

    RHIShaderPermutationSet::KeyBuilder& RHIShaderPermutationSet::KeyBuilder::SetBool(const char* dimensionName,
                                                                                      bool value)
    {
        return Set(dimensionName, value ? 1 : 0);
    }

    //=========================================================================
    // RHIShaderManager
    //=========================================================================

    RHIShaderManager::~RHIShaderManager()
    {
        Shutdown();
    }

    bool RHIShaderManager::Initialize(IRHIDevice* device)
    {
        m_device = device;
        m_hotReloadEnabled = false;
        return true;
    }

    void RHIShaderManager::Shutdown()
    {
        m_shaderCache.clear();
        m_device = nullptr;
    }

    IRHIShader* RHIShaderManager::LoadShader(const char* path, EShaderFrequency frequency, const char* entryPoint)
    {
        if ((m_device == nullptr) || (path == nullptr)) {
            return nullptr;
}

        // キャッシュ検索
        auto it = m_shaderCache.find(path);
        if (it != m_shaderCache.end()) {
            return it->second.Get();
}

        // ロードコールバック経由でバイトコード取得
        if (!m_loadCallback) {
            return nullptr;
}

        RHIShaderBytecode const bytecode = m_loadCallback(path);
        if (!bytecode.IsValid()) {
            return nullptr;
}

        // シェーダー作成
        RHIShaderDesc desc;
        desc.frequency = frequency;
        desc.bytecode = bytecode;
        desc.entryPoint = entryPoint;
        desc.debugName = path;

        IRHIShader* shader = m_device->CreateShader(desc, path);
        if (shader == nullptr) {
            return nullptr;
}

        m_shaderCache[path] = shader;
        return shader;
    }

    IRHIShaderLibrary* RHIShaderManager::LoadShaderLibrary(const char* path)
    {
        if ((m_device == nullptr) || (path == nullptr) || !m_loadCallback) {
            return nullptr;
}

        RHIShaderBytecode const bytecode = m_loadCallback(path);
        if (!bytecode.IsValid()) {
            return nullptr;
}

        RHIShaderLibraryDesc desc;
        desc.bytecode = bytecode;
        desc.name = path;

        return m_device->CreateShaderLibrary(desc, path);
    }

    RHIShaderPermutationSet* RHIShaderManager::LoadPermutationSet(const char* basePath,
                                                                  const RHIPermutationDimension* dimensions,
                                                                  uint32 dimensionCount)
    {
        // パーミュテーションセットのロードはバックエンド/ファイルシステム依存
        (void)basePath;
        (void)dimensions;
        (void)dimensionCount;
        return nullptr;
    }

    void RHIShaderManager::SetLoadCallback(RHIShaderLoadCallback callback)
    {
        m_loadCallback = std::move(callback);
    }

    void RHIShaderManager::ClearCache()
    {
        m_shaderCache.clear();
    }

    RHIShaderManager::CacheStats RHIShaderManager::GetCacheStats() const
    {
        CacheStats stats;
        stats.totalShaders = static_cast<uint32>(m_shaderCache.size());
        // ヒット/ミスカウントはロード時に実際にトラッキングする必要がある
        return stats;
    }

    void RHIShaderManager::EnableHotReload(bool enable)
    {
        m_hotReloadEnabled = enable;
    }

    void RHIShaderManager::CheckForChanges() const
    {
        if (!m_hotReloadEnabled) {
            return;
}

        // ファイル変更検出はプラットフォーム依存
    }

    uint32 RHIShaderManager::ReloadChangedShaders() const
    {
        if (!m_hotReloadEnabled) {
            return 0;
}

        // 変更検出・リロードはプラットフォーム依存
        return 0;
    }

    void RHIShaderManager::SetShaderChangedCallback(ShaderChangedCallback callback)
    {
        m_changedCallback = std::move(callback);
    }

    //=========================================================================
    // RHIShaderPrecompiler
    //=========================================================================

    RHIPrecompileResult RHIShaderPrecompiler::Precompile(const RHIPrecompileOptions& options)
    {
        RHIPrecompileResult result;

        if ((options.sourceDirectory == nullptr) || (options.outputDirectory == nullptr)) {
            return result;
}

        // プリコンパイルはシェーダーコンパイラ依存
        // 基本フレームワーク: ソースディレクトリを走査 → コンパイル → 出力
        (void)options;

        return result;
    }

    void RHIShaderPrecompiler::SetProgressCallback(ProgressCallback callback)
    {
        m_progressCallback = std::move(callback);
    }

} // namespace NS::RHI
