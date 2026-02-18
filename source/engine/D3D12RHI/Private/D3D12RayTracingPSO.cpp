/// @file D3D12RayTracingPSO.cpp
/// @brief D3D12 レイトレーシング Pipeline State Object 実装
#include "D3D12RayTracingPSO.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"

#include <string>
#include <vector>

namespace NS::D3D12RHI
{
    bool D3D12RaytracingPipelineState::Init(D3D12Device* device,
                                            const NS::RHI::RHIRaytracingPipelineStateDesc& desc,
                                            const char* /*debugName*/)
    {
        if (!device || !device->GetD3DDevice5())
            return false;

        device_ = device;
        globalRootSignature_ = desc.globalRootSignature;
        maxPayloadSize_ = desc.shaderConfig.maxPayloadSize;
        maxAttributeSize_ = desc.shaderConfig.maxAttributeSize;
        maxRecursionDepth_ = desc.pipelineConfig.maxTraceRecursionDepth;

        // サブオブジェクト数を計算
        // libraries + hitGroups + shaderConfig + pipelineConfig + globalRootSig
        // + localRootSig(each: LOCAL_ROOT_SIGNATURE + SUBOBJECT_TO_EXPORTS_ASSOCIATION)
        // + DXIL_LIBRARY export arrays
        uint32 subobjectCount = 0;
        subobjectCount += desc.libraryCount;  // DXIL libraries
        subobjectCount += desc.hitGroupCount; // Hit groups
        subobjectCount += 1;                  // Shader config
        subobjectCount += 1;                  // Pipeline config
        if (desc.globalRootSignature)
            subobjectCount += 1;                            // Global root signature
        subobjectCount += desc.localRootSignatureCount * 2; // Local RS + association pairs

        std::vector<D3D12_STATE_SUBOBJECT> subobjects(subobjectCount);
        uint32 idx = 0;

        // --- DXIL Libraries ---
        std::vector<D3D12_DXIL_LIBRARY_DESC> dxilLibs(desc.libraryCount);
        std::vector<std::vector<D3D12_EXPORT_DESC>> exportArrays(desc.libraryCount);

        for (uint32 i = 0; i < desc.libraryCount; ++i)
        {
            const auto& lib = desc.libraries[i];
            dxilLibs[i].DXILLibrary.pShaderBytecode = lib.bytecode.data;
            dxilLibs[i].DXILLibrary.BytecodeLength = static_cast<SIZE_T>(lib.bytecode.size);

            if (lib.exportNames && lib.exportCount > 0)
            {
                exportArrays[i].resize(lib.exportCount);
                for (uint32 j = 0; j < lib.exportCount; ++j)
                {
                    exportArrays[i][j] = {};
                    // Convert narrow to wide string for D3D12
                    // Store temporarily — we need wchar_t* for D3D12
                    exportArrays[i][j].Name = nullptr; // Will be set below
                    exportArrays[i][j].ExportToRename = nullptr;
                    exportArrays[i][j].Flags = D3D12_EXPORT_FLAG_NONE;
                }
                dxilLibs[i].NumExports = lib.exportCount;
                // Note: export names need wchar conversion, handle below
            }
            else
            {
                dxilLibs[i].NumExports = 0;
                dxilLibs[i].pExports = nullptr;
            }

            subobjects[idx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            subobjects[idx].pDesc = &dxilLibs[i];
            ++idx;
        }

        // Export name wchar conversion storage
        std::vector<std::vector<std::wstring>> wideExportNames(desc.libraryCount);
        for (uint32 i = 0; i < desc.libraryCount; ++i)
        {
            const auto& lib = desc.libraries[i];
            if (lib.exportNames && lib.exportCount > 0)
            {
                wideExportNames[i].resize(lib.exportCount);
                for (uint32 j = 0; j < lib.exportCount; ++j)
                {
                    const char* name = lib.exportNames[j];
                    int len = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
                    wideExportNames[i][j].resize(len - 1);
                    MultiByteToWideChar(CP_UTF8, 0, name, -1, wideExportNames[i][j].data(), len);
                    exportArrays[i][j].Name = wideExportNames[i][j].c_str();
                }
                dxilLibs[i].pExports = exportArrays[i].data();
            }
        }

        // --- Hit Groups ---
        std::vector<D3D12_HIT_GROUP_DESC> hitGroups(desc.hitGroupCount);
        std::vector<std::wstring> hitGroupNames(desc.hitGroupCount);
        std::vector<std::wstring> closestHitNames(desc.hitGroupCount);
        std::vector<std::wstring> anyHitNames(desc.hitGroupCount);
        std::vector<std::wstring> intersectionNames(desc.hitGroupCount);

        auto toWide = [](const char* narrow, std::wstring& wide) -> const wchar_t* {
            if (!narrow)
                return nullptr;
            int len = MultiByteToWideChar(CP_UTF8, 0, narrow, -1, nullptr, 0);
            wide.resize(len - 1);
            MultiByteToWideChar(CP_UTF8, 0, narrow, -1, wide.data(), len);
            return wide.c_str();
        };

        for (uint32 i = 0; i < desc.hitGroupCount; ++i)
        {
            const auto& hg = desc.hitGroups[i];
            hitGroups[i] = {};
            hitGroups[i].HitGroupExport = toWide(hg.hitGroupName, hitGroupNames[i]);
            hitGroups[i].ClosestHitShaderImport = toWide(hg.closestHitShaderName, closestHitNames[i]);
            hitGroups[i].AnyHitShaderImport = toWide(hg.anyHitShaderName, anyHitNames[i]);
            hitGroups[i].IntersectionShaderImport = toWide(hg.intersectionShaderName, intersectionNames[i]);
            hitGroups[i].Type =
                hg.IsProceduralHitGroup() ? D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE : D3D12_HIT_GROUP_TYPE_TRIANGLES;

            subobjects[idx].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            subobjects[idx].pDesc = &hitGroups[i];
            ++idx;
        }

        // --- Shader Config ---
        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig{};
        shaderConfig.MaxPayloadSizeInBytes = desc.shaderConfig.maxPayloadSize;
        shaderConfig.MaxAttributeSizeInBytes = desc.shaderConfig.maxAttributeSize;
        subobjects[idx].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        subobjects[idx].pDesc = &shaderConfig;
        ++idx;

        // --- Pipeline Config ---
        D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig{};
        pipelineConfig.MaxTraceRecursionDepth = desc.pipelineConfig.maxTraceRecursionDepth;
        subobjects[idx].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        subobjects[idx].pDesc = &pipelineConfig;
        ++idx;

        // --- Global Root Signature ---
        D3D12_GLOBAL_ROOT_SIGNATURE globalRS{};
        if (desc.globalRootSignature)
        {
            auto* d3dRS = static_cast<D3D12RootSignature*>(desc.globalRootSignature);
            globalRS.pGlobalRootSignature = d3dRS->GetD3DRootSignature();
            subobjects[idx].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
            subobjects[idx].pDesc = &globalRS;
            ++idx;
        }

        // --- Local Root Signatures ---
        std::vector<D3D12_LOCAL_ROOT_SIGNATURE> localRSDescs(desc.localRootSignatureCount);
        std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> localAssocs(desc.localRootSignatureCount);
        std::vector<std::vector<std::wstring>> localExportWideNames(desc.localRootSignatureCount);
        std::vector<std::vector<LPCWSTR>> localExportPtrs(desc.localRootSignatureCount);

        for (uint32 i = 0; i < desc.localRootSignatureCount; ++i)
        {
            const auto& assoc = desc.localRootSignatures[i];

            // Local Root Signature subobject
            auto* d3dLocalRS = static_cast<D3D12RootSignature*>(assoc.localRootSignature);
            localRSDescs[i].pLocalRootSignature = d3dLocalRS ? d3dLocalRS->GetD3DRootSignature() : nullptr;
            subobjects[idx].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
            subobjects[idx].pDesc = &localRSDescs[i];
            uint32 localRSIdx = idx;
            ++idx;

            // Association subobject
            localExportWideNames[i].resize(assoc.associatedExportCount);
            localExportPtrs[i].resize(assoc.associatedExportCount);
            for (uint32 j = 0; j < assoc.associatedExportCount; ++j)
            {
                toWide(assoc.associatedExportNames[j], localExportWideNames[i][j]);
                localExportPtrs[i][j] = localExportWideNames[i][j].c_str();
            }

            localAssocs[i].pSubobjectToAssociate = &subobjects[localRSIdx];
            localAssocs[i].pExports = localExportPtrs[i].data();
            localAssocs[i].NumExports = assoc.associatedExportCount;
            subobjects[idx].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            subobjects[idx].pDesc = &localAssocs[i];
            ++idx;
        }

        // --- StateObject作成 ---
        D3D12_STATE_OBJECT_DESC stateObjectDesc{};
        stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        stateObjectDesc.NumSubobjects = idx;
        stateObjectDesc.pSubobjects = subobjects.data();

        HRESULT hr = device->GetD3DDevice5()->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&stateObject_));
        if (FAILED(hr))
        {
            char buf[128];
            snprintf(
                buf, sizeof(buf), "[D3D12RHI] CreateStateObject (RTPSO) failed: 0x%08X", static_cast<unsigned>(hr));
            LOG_ERROR(buf);
            return false;
        }

        // Properties取得（シェーダー識別子検索用）
        hr = stateObject_->QueryInterface(IID_PPV_ARGS(&properties_));
        if (FAILED(hr))
        {
            LOG_ERROR("[D3D12RHI] Failed to get StateObjectProperties");
            return false;
        }

        return true;
    }

    NS::RHI::RHIShaderIdentifier D3D12RaytracingPipelineState::GetShaderIdentifier(const char* exportName) const
    {
        NS::RHI::RHIShaderIdentifier result{};
        if (!properties_ || !exportName)
            return result;

        // Convert to wide string
        int len = MultiByteToWideChar(CP_UTF8, 0, exportName, -1, nullptr, 0);
        std::wstring wideName(len - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, exportName, -1, wideName.data(), len);

        const void* id = properties_->GetShaderIdentifier(wideName.c_str());
        if (id)
        {
            memcpy(result.data, id, NS::RHI::kShaderIdentifierSize);
        }

        return result;
    }

} // namespace NS::D3D12RHI
