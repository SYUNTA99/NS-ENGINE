//----------------------------------------------------------------------------
//! @file   gpu_resource.h
//! @brief  GPUリソース共通定義
//----------------------------------------------------------------------------
#pragma once

#include "dx11/gpu_common.h"
#include <stdexcept>

//! 16バイトアライメント
constexpr size_t kGpuAlignment = 16;

//! サイズを16バイト境界にアライン
constexpr size_t AlignGpuSize(size_t size) noexcept {
    return (size + kGpuAlignment - 1) & ~(kGpuAlignment - 1);
}

//===========================================================================
//! @brief DirectXエラー例外
//===========================================================================
class DxException : public std::runtime_error {
public:
    DxException(HRESULT hr, const char* file, int line)
        : std::runtime_error(Format(hr, file, line)), hr_(hr) {}

    [[nodiscard]] HRESULT Code() const noexcept { return hr_; }

private:
    HRESULT hr_;

    static std::string Format(HRESULT hr, const char* file, int line) {
        char buf[256];
        snprintf(buf, sizeof(buf), "DX11 Error 0x%08X at %s:%d",
                 static_cast<unsigned>(hr), file, line);
        return buf;
    }
};

//! HRESULTチェックマクロ
#define DX_CHECK(hr) do { \
    HRESULT _hr = (hr); \
    if (FAILED(_hr)) throw DxException(_hr, __FILE__, __LINE__); \
} while(0)
