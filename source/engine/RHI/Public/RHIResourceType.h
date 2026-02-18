/// @file RHIResourceType.h
/// @brief リソースタイプ分類・型安全キャストユーティリティ
/// @details ERHIResourceTypeの分類関数、DECLARE_RHI_RESOURCE_TYPE マクロ、RHICast/IsA を提供。
/// @see 01-13-resource-type.md
#pragma once

#include "IRHIResource.h"
#include "RHIRefCountPtr.h"

namespace NS
{
    namespace RHI
    {
        //=========================================================================
        // リソースタイプ名取得
        //=========================================================================

        /// リソースタイプ名取得
        inline const char* GetResourceTypeName(ERHIResourceType type)
        {
            switch (type)
            {
            case ERHIResourceType::Buffer:
                return "Buffer";
            case ERHIResourceType::Texture:
                return "Texture";
            case ERHIResourceType::ShaderResourceView:
                return "SRV";
            case ERHIResourceType::UnorderedAccessView:
                return "UAV";
            case ERHIResourceType::RenderTargetView:
                return "RTV";
            case ERHIResourceType::DepthStencilView:
                return "DSV";
            case ERHIResourceType::ConstantBufferView:
                return "CBV";
            case ERHIResourceType::Sampler:
                return "Sampler";
            case ERHIResourceType::Shader:
                return "Shader";
            case ERHIResourceType::GraphicsPipelineState:
                return "GraphicsPSO";
            case ERHIResourceType::ComputePipelineState:
                return "ComputePSO";
            case ERHIResourceType::RootSignature:
                return "RootSignature";
            case ERHIResourceType::CommandList:
                return "CommandList";
            case ERHIResourceType::CommandAllocator:
                return "CommandAllocator";
            case ERHIResourceType::Fence:
                return "Fence";
            case ERHIResourceType::SyncPoint:
                return "SyncPoint";
            case ERHIResourceType::DescriptorHeap:
                return "DescriptorHeap";
            case ERHIResourceType::QueryHeap:
                return "QueryHeap";
            case ERHIResourceType::SwapChain:
                return "SwapChain";
            case ERHIResourceType::AccelerationStructure:
                return "AccelerationStructure";
            case ERHIResourceType::RayTracingPSO:
                return "RayTracingPSO";
            case ERHIResourceType::ShaderBindingTable:
                return "ShaderBindingTable";
            case ERHIResourceType::Heap:
                return "Heap";
            case ERHIResourceType::InputLayout:
                return "InputLayout";
            case ERHIResourceType::ShaderLibrary:
                return "ShaderLibrary";
            case ERHIResourceType::MeshPipelineState:
                return "MeshPipelineState";
            case ERHIResourceType::ResourceCollection:
                return "ResourceCollection";
            default:
                return "Unknown";
            }
        }

        //=========================================================================
        // リソースタイプ分類
        //=========================================================================

        /// GPUメモリを占有するリソースか
        inline bool IsGPUResource(ERHIResourceType type)
        {
            switch (type)
            {
            case ERHIResourceType::Buffer:
            case ERHIResourceType::Texture:
            case ERHIResourceType::AccelerationStructure:
                return true;
            default:
                return false;
            }
        }

        /// ビュータイプか
        inline bool IsViewType(ERHIResourceType type)
        {
            switch (type)
            {
            case ERHIResourceType::ShaderResourceView:
            case ERHIResourceType::UnorderedAccessView:
            case ERHIResourceType::RenderTargetView:
            case ERHIResourceType::DepthStencilView:
            case ERHIResourceType::ConstantBufferView:
                return true;
            default:
                return false;
            }
        }

        /// パイプラインステートか
        inline bool IsPipelineState(ERHIResourceType type)
        {
            switch (type)
            {
            case ERHIResourceType::GraphicsPipelineState:
            case ERHIResourceType::ComputePipelineState:
            case ERHIResourceType::RayTracingPSO:
            case ERHIResourceType::MeshPipelineState:
                return true;
            default:
                return false;
            }
        }

        /// コマンド関連か
        inline bool IsCommandResource(ERHIResourceType type)
        {
            return type == ERHIResourceType::CommandList || type == ERHIResourceType::CommandAllocator;
        }

        //=========================================================================
        // DECLARE_RHI_RESOURCE_TYPE マクロ
        //=========================================================================

        /// 静的リソースタイプ取得用マクロ
        /// 各リソースクラスで使用:
        ///   DECLARE_RHI_RESOURCE_TYPE(Buffer)
#define DECLARE_RHI_RESOURCE_TYPE(TypeName)                                                                            \
    static ERHIResourceType StaticResourceType()                                                                       \
    {                                                                                                                  \
        return ERHIResourceType::TypeName;                                                                             \
    }                                                                                                                  \
    ERHIResourceType GetResourceType() const noexcept                                                                  \
    {                                                                                                                  \
        return ERHIResourceType::TypeName;                                                                             \
    }

        //=========================================================================
        // RHICast: 型チェック付きキャスト
        //=========================================================================

        /// 型チェック付きダウンキャスト
        template <typename T> T* RHICast(IRHIResource* resource)
        {
            if (resource && resource->GetResourceType() == T::StaticResourceType())
            {
                return static_cast<T*>(resource);
            }
            return nullptr;
        }

        /// const版
        template <typename T> const T* RHICast(const IRHIResource* resource)
        {
            if (resource && resource->GetResourceType() == T::StaticResourceType())
            {
                return static_cast<const T*>(resource);
            }
            return nullptr;
        }

        /// TRefCountPtr版
        template <typename T> TRefCountPtr<T> RHICast(const TRefCountPtr<IRHIResource>& resource)
        {
            if (resource && resource->GetResourceType() == T::StaticResourceType())
            {
                return TRefCountPtr<T>(static_cast<T*>(resource.Get()));
            }
            return nullptr;
        }

        //=========================================================================
        // IsA: 型チェック（キャストなし）
        //=========================================================================

        /// 型チェック（キャストなし）
        template <typename T> bool IsA(const IRHIResource* resource)
        {
            return resource && resource->GetResourceType() == T::StaticResourceType();
        }

        /// TRefCountPtr版
        template <typename T, typename U> bool IsA(const TRefCountPtr<U>& resource)
        {
            return resource && resource->GetResourceType() == T::StaticResourceType();
        }

        /// 指定タイプか確認
        inline bool IsResourceType(const IRHIResource* resource, ERHIResourceType type)
        {
            return resource && resource->GetResourceType() == type;
        }

    } // namespace RHI
} // namespace NS
