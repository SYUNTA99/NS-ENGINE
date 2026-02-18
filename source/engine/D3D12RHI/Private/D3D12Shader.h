/// @file D3D12Shader.h
/// @brief D3D12シェーダー実装
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIShader.h"

#include <vector>

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12Shader
    //=========================================================================

    /// D3D12シェーダーリソース
    /// コンパイル済みDXILバイトコードを保持
    class D3D12Shader final : public NS::RHI::IRHIShader
    {
    public:
        D3D12Shader() = default;
        ~D3D12Shader() override = default;

        /// 初期化（RHIShaderDescから）
        bool Init(D3D12Device* device, const NS::RHI::RHIShaderDesc& desc, const char* debugName);

        //=====================================================================
        // IRHIShader
        //=====================================================================

        NS::RHI::IRHIDevice* GetDevice() const override;
        NS::RHI::EShaderFrequency GetFrequency() const override { return frequency_; }
        NS::RHI::RHIShaderModel GetShaderModel() const override { return shaderModel_; }
        const char* GetEntryPoint() const override { return entryPoint_.c_str(); }
        NS::RHI::RHIShaderHash GetHash() const override { return hash_; }
        NS::RHI::RHIShaderBytecode GetBytecode() const override;

    private:
        D3D12Device* device_ = nullptr;
        NS::RHI::EShaderFrequency frequency_ = NS::RHI::EShaderFrequency::Vertex;
        NS::RHI::RHIShaderModel shaderModel_;
        std::string entryPoint_;
        NS::RHI::RHIShaderHash hash_;
        std::vector<uint8> bytecodeData_;
    };

    //=========================================================================
    // D3D12ShaderCompiler
    //=========================================================================

    /// DXCベースのHLSLコンパイラ
    /// dxcompiler.dllを動的ロードして使用
    class D3D12ShaderCompiler
    {
    public:
        D3D12ShaderCompiler() = default;
        ~D3D12ShaderCompiler();

        /// 初期化（dxcompiler.dllロード）
        bool Init();

        /// シャットダウン
        void Shutdown();

        /// HLSLソースコードをコンパイル
        NS::RHI::RHIShaderCompileResult CompileFromSource(
            const char* source,
            uint32 sourceSize,
            const char* sourceName,
            const char* entryPoint,
            NS::RHI::EShaderFrequency frequency,
            const NS::RHI::RHIShaderCompileOptions& options);

        /// 初期化済みか
        bool IsInitialized() const { return dxcDll_ != nullptr; }

    private:
        void* dxcDll_ = nullptr;   // HMODULE
        void* compiler_ = nullptr; // IDxcCompiler3*
        void* utils_ = nullptr;    // IDxcUtils*
    };

} // namespace NS::D3D12RHI
