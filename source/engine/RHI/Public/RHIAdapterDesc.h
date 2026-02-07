/// @file RHIAdapterDesc.h
/// @brief GPUアダプター記述情報
/// @details GPU識別、メモリ、機能レベル、機能フラグを保持する構造体。
/// @see 01-18-adapter-desc.md
#pragma once

#include "Common/Utility/Types.h"
#include "RHIEnums.h"
#include "RHIMacros.h"
#include <string>

namespace NS::RHI
{
    //=========================================================================
    // ベンダーID定数
    //=========================================================================

    constexpr uint32 kVendorNVIDIA = 0x10DE;
    constexpr uint32 kVendorAMD = 0x1002;
    constexpr uint32 kVendorIntel = 0x8086;
    constexpr uint32 kVendorQualcomm = 0x5143;
    constexpr uint32 kVendorARM = 0x13B5;
    constexpr uint32 kVendorImgTech = 0x1010;
    constexpr uint32 kVendorMicrosoft = 0x1414; // WARP
    constexpr uint32 kVendorApple = 0x106B;

    /// ベンダー名取得
    inline const char* GetVendorName(uint32 vendorId)
    {
        switch (vendorId)
        {
        case kVendorNVIDIA:
            return "NVIDIA";
        case kVendorAMD:
            return "AMD";
        case kVendorIntel:
            return "Intel";
        case kVendorQualcomm:
            return "Qualcomm";
        case kVendorARM:
            return "ARM";
        case kVendorImgTech:
            return "Imagination Technologies";
        case kVendorMicrosoft:
            return "Microsoft";
        case kVendorApple:
            return "Apple";
        default:
            return "Unknown";
        }
    }

    //=========================================================================
    // RHIAdapterDesc
    //=========================================================================

    /// アダプター記述情報
    struct RHI_API RHIAdapterDesc
    {
        //=====================================================================
        // 識別情報
        //=====================================================================

        uint32 adapterIndex = 0;
        std::string deviceName;
        uint32 vendorId = 0;
        uint32 deviceId = 0;
        uint32 subsystemId = 0;
        uint32 revision = 0;
        uint64 driverVersion = 0;

        //=====================================================================
        // メモリ情報
        //=====================================================================

        uint64 dedicatedVideoMemory = 0;
        uint64 dedicatedSystemMemory = 0;
        uint64 sharedSystemMemory = 0;
        bool unifiedMemory = false;

        //=====================================================================
        // 機能レベル
        //=====================================================================

        ERHIFeatureLevel maxFeatureLevel = ERHIFeatureLevel::SM5;
        EShaderModel maxShaderModel = EShaderModel::SM5_0;
        uint32 resourceBindingTier = 0;
        uint32 resourceHeapTier = 0;

        //=====================================================================
        // GPUノード
        //=====================================================================

        uint32 numDeviceNodes = 1;
        bool isLinkedAdapter = false;

        //=====================================================================
        // 機能フラグ
        //=====================================================================

        bool supportsRayTracing = false;
        bool supportsMeshShaders = false;
        bool supportsBindless = false;
        bool supportsVariableRateShading = false;
        bool supportsWaveOperations = false;
        bool supports64BitAtomics = false;
        bool isTileBased = false;
        bool isDiscreteGPU = true;
        bool isSoftwareAdapter = false;

        //=====================================================================
        // ユーティリティ
        //=====================================================================

        bool IsNVIDIA() const { return vendorId == kVendorNVIDIA; }
        bool IsAMD() const { return vendorId == kVendorAMD; }
        bool IsIntel() const { return vendorId == kVendorIntel; }
        bool IsIntegrated() const { return !isDiscreteGPU; }

        uint64 GetTotalVideoMemory() const { return dedicatedVideoMemory + sharedSystemMemory; }
    };

} // namespace NS::RHI
