/// @file IRHIShader.cpp
/// @brief シェーダーハッシュ・シェーダーモデル実装
#include "IRHIShader.h"
#include <cstdio>
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHIShaderModel
    //=========================================================================

    const char* RHIShaderModel::ToString() const
    {
        // スレッドローカルバッファで文字列を返す
        thread_local static char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "SM_%u_%u", major, minor);
        return buffer;
    }

    //=========================================================================
    // RHIShaderHash
    //=========================================================================

    std::string RHIShaderHash::ToString() const
    {
        char buffer[33];
        std::snprintf(buffer,
                      sizeof(buffer),
                      "%016llx%016llx",
                      static_cast<unsigned long long>(hash[0]),
                      static_cast<unsigned long long>(hash[1]));
        return std::string(buffer);
    }

    RHIShaderHash RHIShaderHash::FromString(const char* str)
    {
        RHIShaderHash result;
        if ((str == nullptr) || std::strlen(str) < 32) {
            return result;
}

        // 上位64bitをパース
        char upper[17] = {};
        std::memcpy(upper, str, 16);
        result.hash[0] = std::strtoull(upper, nullptr, 16);

        // 下位64bitをパース
        char lower[17] = {};
        std::memcpy(lower, str + 16, 16);
        result.hash[1] = std::strtoull(lower, nullptr, 16);

        return result;
    }

    RHIShaderHash RHIShaderHash::Compute(const void* data, MemorySize size)
    {
        RHIShaderHash result;
        if ((data == nullptr) || size == 0) {
            return result;
}

        // FNV-1a ハッシュ（2つの独立したハッシュで128bit生成）
        const auto* bytes = static_cast<const uint8*>(data);

        // 第1ハッシュ（FNV-1a標準）
        uint64 h0 = 14695981039346656037ULL;
        for (MemorySize i = 0; i < size; ++i)
        {
            h0 ^= static_cast<uint64>(bytes[i]);
            h0 *= 1099511628211ULL;
        }
        result.hash[0] = h0;

        // 第2ハッシュ（異なるシードで独立性を確保）
        uint64 h1 = 0xcbf29ce484222325ULL ^ 0xdeadbeef;
        for (MemorySize i = 0; i < size; ++i)
        {
            h1 ^= static_cast<uint64>(bytes[size - 1 - i]);
            h1 *= 1099511628211ULL;
        }
        result.hash[1] = h1;

        return result;
    }

    //=========================================================================
    // RHIShaderCacheKey
    //=========================================================================

    bool RHIShaderCacheKey::operator==(const RHIShaderCacheKey& other) const
    {
        return sourceHash == other.sourceHash && shaderModel == other.shaderModel && frequency == other.frequency &&
               compileOptionsHash == other.compileOptionsHash;
    }

    bool RHIShaderCacheKey::operator<(const RHIShaderCacheKey& other) const
    {
        if (sourceHash != other.sourceHash) {
            return sourceHash < other.sourceHash;
}
        if (shaderModel != other.shaderModel) {
            return shaderModel < other.shaderModel;
}
        if (frequency != other.frequency) {
            return frequency < other.frequency;
}
        return compileOptionsHash < other.compileOptionsHash;
    }

} // namespace NS::RHI
