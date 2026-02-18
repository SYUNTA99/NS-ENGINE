/// @file D3D12WorkGraph.cpp
/// @brief D3D12 Work Graphs パイプライン実装
#include "D3D12WorkGraph.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"

#include <string>
#include <vector>

namespace NS::D3D12RHI
{
    bool D3D12WorkGraphPipeline::Init(D3D12Device* device, const NS::RHI::RHIWorkGraphPipelineDesc& desc)
    {
        if (!device)
            return false;

        device_ = device;

#ifdef D3D12_STATE_OBJECT_TYPE_EXECUTABLE
        auto* d3dDevice = device->GetD3DDevice5();
        if (!d3dDevice)
            return false;

        // サブオブジェクト構築
        // 最低: グローバルRS + WorkGraphConfig
        std::vector<D3D12_STATE_SUBOBJECT> subobjects;
        subobjects.reserve(4);

        // グローバルルートシグネチャ
        D3D12_GLOBAL_ROOT_SIGNATURE globalRS{};
        if (desc.globalRootSignature)
        {
            auto* d3dRS = static_cast<D3D12RootSignature*>(desc.globalRootSignature);
            globalRS.pGlobalRootSignature = d3dRS->GetD3DRootSignature();
            D3D12_STATE_SUBOBJECT so{};
            so.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
            so.pDesc = &globalRS;
            subobjects.push_back(so);
        }

        // State Object作成
        D3D12_STATE_OBJECT_DESC stateObjectDesc{};
        stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_EXECUTABLE;
        stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
        stateObjectDesc.pSubobjects = subobjects.data();

        HRESULT hr = d3dDevice->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&stateObject_));
        if (FAILED(hr))
        {
            char buf[128];
            snprintf(
                buf, sizeof(buf), "[D3D12RHI] CreateStateObject (WorkGraph) failed: 0x%08X", static_cast<unsigned>(hr));
            LOG_ERROR(buf);
            return false;
        }

        // WorkGraphProperties取得
        ComPtr<ID3D12WorkGraphProperties> wgProps;
        if (SUCCEEDED(stateObject_->QueryInterface(IID_PPV_ARGS(&wgProps))))
        {
            // プログラムインデックス検索
            UINT programIndex = 0;
            if (desc.programName)
            {
                int len = MultiByteToWideChar(CP_UTF8, 0, desc.programName, -1, nullptr, 0);
                std::wstring wideName(len - 1, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, desc.programName, -1, wideName.data(), len);
                programIndex = wgProps->GetWorkGraphIndex(wideName.c_str());
            }

            nodeCount_ = wgProps->GetNumNodes(programIndex);
            entryPointCount_ = wgProps->GetNumEntrypoints(programIndex);

            // メモリ要件取得
            D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memReqs{};
            wgProps->GetWorkGraphMemoryRequirements(programIndex, &memReqs);
            backingMemorySize_ = memReqs.MaxSizeInBytes;

            // Program ID = programIndex (simple mapping)
            programId_ = programIndex;
        }

        return true;
#else
        (void)desc;
        LOG_ERROR("[D3D12RHI] Work Graphs not supported by SDK");
        return false;
#endif
    }

    int32 D3D12WorkGraphPipeline::GetNodeIndex(const char* /*nodeName*/) const
    {
        // Work Graph node index lookup requires ID3D12WorkGraphProperties
        // which is only available with the right SDK version
        return -1;
    }

} // namespace NS::D3D12RHI
