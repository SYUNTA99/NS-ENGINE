/// @file D3D12Shader.cpp
/// @brief D3D12シェーダー実装
#include "D3D12Shader.h"

#include "D3D12Device.h"

#include <dxcapi.h>

#include <cstring>

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12Shader
    //=========================================================================

    bool D3D12Shader::Init(D3D12Device* device, const NS::RHI::RHIShaderDesc& desc, const char* debugName)
    {
        if (!device || !desc.bytecode.IsValid())
            return false;

        device_ = device;
        frequency_ = desc.frequency;
        shaderModel_ = desc.shaderModel;
        entryPoint_ = desc.entryPoint ? desc.entryPoint : "main";

        // バイトコードをコピー（所有権を持つ）
        const auto* src = static_cast<const uint8*>(desc.bytecode.data);
        bytecodeData_.assign(src, src + desc.bytecode.size);

        // ハッシュ計算
        hash_ = NS::RHI::RHIShaderHash::Compute(bytecodeData_.data(),
                                                  static_cast<NS::RHI::MemorySize>(bytecodeData_.size()));

        // デバッグ名設定
        if (debugName)
            SetDebugName(debugName);

        return true;
    }

    NS::RHI::IRHIDevice* D3D12Shader::GetDevice() const
    {
        return device_;
    }

    NS::RHI::RHIShaderBytecode D3D12Shader::GetBytecode() const
    {
        return NS::RHI::RHIShaderBytecode::FromData(bytecodeData_.data(),
                                                     static_cast<NS::RHI::MemorySize>(bytecodeData_.size()));
    }

    //=========================================================================
    // D3D12ShaderCompiler
    //=========================================================================

    D3D12ShaderCompiler::~D3D12ShaderCompiler()
    {
        Shutdown();
    }

    bool D3D12ShaderCompiler::Init()
    {
        if (dxcDll_)
            return true;

        // dxcompiler.dllを動的ロード
        HMODULE dll = LoadLibraryW(L"dxcompiler.dll");
        if (!dll)
        {
            LOG_WARN("[D3D12RHI] dxcompiler.dll not found. HLSL compilation unavailable.");
            return false;
        }

        using DxcCreateInstanceFn = HRESULT(WINAPI*)(REFCLSID, REFIID, void**);
        auto createInstance = reinterpret_cast<DxcCreateInstanceFn>(GetProcAddress(dll, "DxcCreateInstance"));
        if (!createInstance)
        {
            FreeLibrary(dll);
            LOG_ERROR("[D3D12RHI] DxcCreateInstance not found in dxcompiler.dll");
            return false;
        }

        IDxcCompiler3* compiler = nullptr;
        HRESULT hr = createInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        if (FAILED(hr))
        {
            FreeLibrary(dll);
            LOG_HRESULT(hr, "[D3D12RHI] Failed to create IDxcCompiler3");
            return false;
        }

        IDxcUtils* utils = nullptr;
        hr = createInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
        if (FAILED(hr))
        {
            compiler->Release();
            FreeLibrary(dll);
            LOG_HRESULT(hr, "[D3D12RHI] Failed to create IDxcUtils");
            return false;
        }

        dxcDll_ = dll;
        compiler_ = compiler;
        utils_ = utils;

        LOG_INFO("[D3D12RHI] DXC shader compiler initialized");
        return true;
    }

    void D3D12ShaderCompiler::Shutdown()
    {
        if (compiler_)
        {
            static_cast<IDxcCompiler3*>(compiler_)->Release();
            compiler_ = nullptr;
        }
        if (utils_)
        {
            static_cast<IDxcUtils*>(utils_)->Release();
            utils_ = nullptr;
        }
        if (dxcDll_)
        {
            FreeLibrary(static_cast<HMODULE>(dxcDll_));
            dxcDll_ = nullptr;
        }
    }

    NS::RHI::RHIShaderCompileResult D3D12ShaderCompiler::CompileFromSource(
        const char* source,
        uint32 sourceSize,
        const char* sourceName,
        const char* entryPoint,
        NS::RHI::EShaderFrequency frequency,
        const NS::RHI::RHIShaderCompileOptions& options)
    {
        using namespace NS::RHI;

        RHIShaderCompileResult result;
        result.success = false;

        if (!dxcDll_ || !compiler_ || !utils_)
        {
            RHIShaderCompileError err;
            err.message = "DXC compiler not initialized";
            result.errors.push_back(err);
            return result;
        }

        auto* compiler = static_cast<IDxcCompiler3*>(compiler_);
        auto* utils = static_cast<IDxcUtils*>(utils_);

        // ターゲットプロファイル名を生成
        char targetBuf[16];
        GetShaderTargetName(frequency, options.shaderModel, targetBuf, sizeof(targetBuf));

        // ソースをワイド文字列に変換（ファイル名）
        wchar_t wSourceName[256] = {};
        if (sourceName)
        {
            for (int i = 0; i < 255 && sourceName[i]; ++i)
                wSourceName[i] = static_cast<wchar_t>(sourceName[i]);
        }

        wchar_t wEntryPoint[128] = {};
        if (entryPoint)
        {
            for (int i = 0; i < 127 && entryPoint[i]; ++i)
                wEntryPoint[i] = static_cast<wchar_t>(entryPoint[i]);
        }

        wchar_t wTarget[16] = {};
        for (int i = 0; i < 15 && targetBuf[i]; ++i)
            wTarget[i] = static_cast<wchar_t>(targetBuf[i]);

        // コンパイル引数を構築
        std::vector<LPCWSTR> args;
        args.push_back(wSourceName);
        args.push_back(L"-E");
        args.push_back(wEntryPoint);
        args.push_back(L"-T");
        args.push_back(wTarget);

        // 最適化レベル
        switch (options.optimization)
        {
        case ERHIShaderOptimization::None:
            args.push_back(L"-Od");
            break;
        case ERHIShaderOptimization::Level1:
            args.push_back(L"-O1");
            break;
        case ERHIShaderOptimization::Level2:
            args.push_back(L"-O2");
            break;
        case ERHIShaderOptimization::Level3:
            args.push_back(L"-O3");
            break;
        }

        // デバッグ情報
        if (options.includeDebugInfo)
        {
            args.push_back(L"-Zi");
            args.push_back(L"-Qembed_debug");
        }

        // 警告をエラーとして扱う
        if (options.warningsAsErrors)
            args.push_back(L"-WX");

        // 行優先行列
        if (options.rowMajorMatrices)
            args.push_back(L"-Zpr");

        // 16ビット型
        if (options.enable16BitTypes)
            args.push_back(L"-enable-16bit-types");

        // プリプロセッサ定義をワイド文字列に変換
        std::vector<std::wstring> defineStrs;
        for (const auto& def : options.defines)
        {
            std::wstring ws = L"-D";
            for (char c : def.first) ws += static_cast<wchar_t>(c);
            ws += L"=";
            for (char c : def.second) ws += static_cast<wchar_t>(c);
            defineStrs.push_back(std::move(ws));
        }
        for (const auto& ws : defineStrs)
            args.push_back(ws.c_str());

        // インクルードパスをワイド文字列に変換
        std::vector<std::wstring> includeStrs;
        for (const auto& inc : options.includePaths)
        {
            std::wstring ws = L"-I";
            for (char c : inc) ws += static_cast<wchar_t>(c);
            includeStrs.push_back(std::move(ws));
        }
        for (const auto& ws : includeStrs)
            args.push_back(ws.c_str());

        // ソースバッファ作成
        IDxcBlobEncoding* sourceBlob = nullptr;
        HRESULT hr = utils->CreateBlob(source, sourceSize, CP_UTF8, &sourceBlob);
        if (FAILED(hr))
        {
            RHIShaderCompileError err;
            err.message = "Failed to create DXC source blob";
            result.errors.push_back(err);
            return result;
        }

        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
        sourceBuffer.Size = sourceBlob->GetBufferSize();
        sourceBuffer.Encoding = CP_UTF8;

        // コンパイル実行
        IDxcResult* compileResult = nullptr;
        hr = compiler->Compile(&sourceBuffer, args.data(), static_cast<UINT32>(args.size()),
                               nullptr, // include handler
                               IID_PPV_ARGS(&compileResult));

        sourceBlob->Release();

        if (FAILED(hr))
        {
            RHIShaderCompileError err;
            err.message = "DXC Compile call failed";
            result.errors.push_back(err);
            return result;
        }

        // コンパイル結果チェック
        HRESULT compileStatus;
        compileResult->GetStatus(&compileStatus);

        // エラー/警告メッセージ取得
        IDxcBlobUtf8* errors = nullptr;
        compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if (errors && errors->GetStringLength() > 0)
        {
            RHIShaderCompileError err;
            err.message = errors->GetStringPointer();
            err.isWarning = SUCCEEDED(compileStatus);
            result.errors.push_back(err);
        }
        if (errors)
            errors->Release();

        if (SUCCEEDED(compileStatus))
        {
            // バイトコード取得
            IDxcBlob* bytecodeBlob = nullptr;
            compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&bytecodeBlob), nullptr);
            if (bytecodeBlob && bytecodeBlob->GetBufferSize() > 0)
            {
                const auto* data = static_cast<const uint8*>(bytecodeBlob->GetBufferPointer());
                result.bytecode.assign(data, data + bytecodeBlob->GetBufferSize());
                result.success = true;
            }
            if (bytecodeBlob)
                bytecodeBlob->Release();
        }

        compileResult->Release();
        return result;
    }

} // namespace NS::D3D12RHI
