/// @file RHIAmplificationShader.h
/// @brief 増幅シェーダー記述・ペイロード・パイプラインペア
/// @details 増幅シェーダーの記述子、ペイロード定義例、メッシュシェーダーとの連携を提供。
/// @see 22-02-amplification-shader.md
#pragma once

#include "IRHIMeshShader.h"
#include "RHIMacros.h"
#include "RHITypes.h"

#include <utility>

namespace NS::RHI
{

    //=========================================================================
    // 定数 (22-02)
    //=========================================================================

    /// 増幅シェーダー出力ペイロード最大サイズ
    constexpr uint32 kRHIMaxAmplificationPayloadSize = 16 * 1024; // 16KB

    //=========================================================================
    // RHIAmplificationShaderDesc (22-02)
    //=========================================================================

    /// 増幅シェーダー記述
    struct RHIAmplificationShaderDesc
    {
        const void* bytecode = nullptr;
        uint64 bytecodeSize = 0;
        uint32 payloadSize = 0; ///< メッシュシェーダーへ渡すペイロードサイズ
        const char* entryPoint = "main";
        const char* debugName = nullptr;
    };

    //=========================================================================
    // RHIAmplificationDispatchInfo (22-02)
    //=========================================================================

    /// 増幅シェーダーディスパッチ情報
    struct RHIAmplificationDispatchInfo
    {
        uint32 meshShaderGroupsX = 0; ///< メッシュシェーダーグループ数X
        uint32 meshShaderGroupsY = 1; ///< メッシュシェーダーグループ数Y
        uint32 meshShaderGroupsZ = 1; ///< メッシュシェーダーグループ数Z
    };

    //=========================================================================
    // ペイロード定義例 (22-02)
    //=========================================================================

    /// LOD選択ペイロード例
    struct RHILODSelectionPayload
    {
        uint32 meshletOffset; ///< 選択されたLODのメッシュレットオフセット
        uint32 meshletCount;  ///< 選択されたLODのメッシュレット数
        uint32 lodLevel;      ///< LODレベル
        uint32 objectId;      ///< オブジェクトID
    };

    /// インスタンスカリングペイロード例
    struct RHIInstanceCullingPayload
    {
        uint32 visibleInstanceIndices[64]; ///< 可視インスタンスインデックス
        uint32 visibleCount;               ///< 可視インスタンス数
    };

    //=========================================================================
    // RHIAmplificationMeshPipeline (22-02)
    //=========================================================================

    /// 増幅メッシュシェーダーペア
    class RHI_API RHIAmplificationMeshPipeline
    {
    public:
        RHIAmplificationMeshPipeline(RHIAmplificationShaderRef amplificationShader, RHIMeshShaderRef meshShader)
            : m_amplificationShader(std::move(amplificationShader)), m_meshShader(std::move(meshShader))
        {}

        IRHIAmplificationShader* GetAmplificationShader() const { return m_amplificationShader.Get(); }

        IRHIMeshShader* GetMeshShader() const { return m_meshShader.Get(); }

        /// ペイロードサイズの互換性チェック
        bool ValidatePayloadCompatibility() const { return m_amplificationShader->GetPayloadSize() > 0; }

    private:
        RHIAmplificationShaderRef m_amplificationShader;
        RHIMeshShaderRef m_meshShader;
    };

} // namespace NS::RHI
