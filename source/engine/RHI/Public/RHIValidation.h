/// @file RHIValidation.h
/// @brief RHI検証レイヤー・検証設定・メッセージ・アサートマクロ
/// @details 検証レベル、カテゴリ、検証メッセージ、検証設定プリセット、
///          デバッグ名ヘルパー、RHIアサートマクロを提供。
/// @see 17-03-validation.md
#pragma once

#include "RHICheck.h"
#include "RHIMacros.h"
#include "RHITypes.h"

#include <cstdarg>
#include <cstdio>

namespace NS::RHI
{
    //=========================================================================
    // ERHIValidationLevel (17-03)
    //=========================================================================

    /// 検証レベル
    enum class ERHIValidationLevel : uint8
    {
        /// 無効
        Disabled,

        /// 基本（致命的なエラーのみ）
        Basic,

        /// 標準（一般的なエラー）
        Standard,

        /// 詳細（パフォーマンス警告含む）
        Verbose,

        /// 最大（すべての検証）
        Maximum,
    };

    //=========================================================================
    // ERHIValidationCategory (17-03)
    //=========================================================================

    /// 検証カテゴリ
    enum class ERHIValidationCategory : uint32
    {
        None = 0,

        /// リソース状態
        ResourceState = 1 << 0,

        /// リソースバインディング
        ResourceBinding = 1 << 1,

        /// コマンドリスト
        CommandList = 1 << 2,

        /// シェーダー
        Shader = 1 << 3,

        /// パイプライン
        Pipeline = 1 << 4,

        /// ディスクリプタ
        Descriptor = 1 << 5,

        /// メモリ
        Memory = 1 << 6,

        /// 同期
        Synchronization = 1 << 7,

        /// スワップチェーン
        SwapChain = 1 << 8,

        /// パフォーマンス
        Performance = 1 << 9,

        /// 全て
        All = 0xFFFFFFFF,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIValidationCategory)

    //=========================================================================
    // ERHIValidationSeverity / RHIValidationMessage (17-03)
    //=========================================================================

    /// 検証メッセージ重要度
    enum class ERHIValidationSeverity : uint8
    {
        Info,
        Warning,
        Error,
        Corruption, // データ破損の可能性
    };

    /// 検証メッセージ
    struct RHI_API RHIValidationMessage
    {
        /// 重要度
        ERHIValidationSeverity severity = ERHIValidationSeverity::Info;

        /// カテゴリ
        ERHIValidationCategory category = ERHIValidationCategory::None;

        /// メッセージID
        uint32 messageId = 0;

        /// メッセージ本文
        char message[1024] = {};

        /// オブジェクト名（関連するリソース等）
        char objectName[256] = {};

        /// 関連オブジェクトポインタ
        void* relatedObject = nullptr;
    };

    /// 検証メッセージコールバック
    using RHIValidationCallback = void (*)(const RHIValidationMessage& message, void* userData);

    //=========================================================================
    // RHIValidationConfig (17-03)
    //=========================================================================

    /// 検証レイヤー設定
    struct RHI_API RHIValidationConfig
    {
        /// 検証レベル
        ERHIValidationLevel level = ERHIValidationLevel::Disabled;

        /// 有効なカテゴリ
        ERHIValidationCategory enabledCategories = ERHIValidationCategory::All;

        /// GPUベース検証（パフォーマンス影響大）
        bool gpuBasedValidation = false;

        /// シェーダーデバッグ情報
        bool shaderDebugInfo = false;

        /// ブレイクオンエラー
        bool breakOnError = false;

        /// ブレイクオン警告
        bool breakOnWarning = false;

        /// メッセージ抑制ID
        const uint32* suppressedMessageIds = nullptr;
        uint32 suppressedMessageCount = 0;

        /// コールバック
        RHIValidationCallback callback = nullptr;
        void* callbackUserData = nullptr;

        //=====================================================================
        // プリセット
        //=====================================================================

        /// デバッグビルド向け
        static RHIValidationConfig Debug()
        {
            RHIValidationConfig config;
            config.level = ERHIValidationLevel::Standard;
            config.gpuBasedValidation = false;
            config.shaderDebugInfo = true;
            config.breakOnError = true;
            return config;
        }

        /// 開発ビルド向け
        static RHIValidationConfig Development()
        {
            RHIValidationConfig config;
            config.level = ERHIValidationLevel::Basic;
            config.shaderDebugInfo = true;
            return config;
        }

        /// リリースビルド向け
        static RHIValidationConfig Release()
        {
            RHIValidationConfig config;
            config.level = ERHIValidationLevel::Disabled;
            return config;
        }

        /// 最大検証（問題調査用）
        static RHIValidationConfig Maximum()
        {
            RHIValidationConfig config;
            config.level = ERHIValidationLevel::Maximum;
            config.gpuBasedValidation = true;
            config.shaderDebugInfo = true;
            config.breakOnError = true;
            return config;
        }
    };

    //=========================================================================
    // ValidationStats (17-03)
    //=========================================================================

    /// 検証統計
    struct RHI_API RHIValidationStats
    {
        uint32 errorCount = 0;
        uint32 warningCount = 0;
        uint32 infoCount = 0;
        uint32 suppressedCount = 0;
    };

    //=========================================================================
    // RHIDebug ヘルパー (17-03)
    //=========================================================================

    /// デバッグマーカー設定ヘルパー
    namespace RHIDebug
    {
        /// リソースにデバッグ名設定
        template <typename T> T* SetName(T* resource, const char* name)
        {
            if (resource)
            {
                resource->SetDebugName(name);
            }
            return resource;
        }

        /// フォーマット付きデバッグ名設定
        template <typename T> T* SetNameF(T* resource, const char* format, ...)
        {
            if (resource)
            {
                char buffer[256];
                va_list args;
                va_start(args, format);
                vsnprintf(buffer, sizeof(buffer), format, args);
                va_end(args);
                resource->SetDebugName(buffer);
            }
            return resource;
        }
    } // namespace RHIDebug

} // namespace NS::RHI

//=============================================================================
// RHIアサートマクロ (17-03)
//=============================================================================

#if NS_RHI_VALIDATION_ENABLED

/// RHIアサート（メッセージ付き）
#define RHI_ASSERT(condition, message)                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            char rhiAssertBuf_[512];                                                                                   \
            snprintf(rhiAssertBuf_, sizeof(rhiAssertBuf_), "[RHI Assert] %s (%s:%d)", message, __FILE__, __LINE__);    \
            LOG_ERROR(rhiAssertBuf_);                                                                                  \
            assert(false && message);                                                                                  \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

/// RHIアサート（フォーマット付き）
#define RHI_ASSERT_F(condition, format, ...)                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            char rhiAssertBuf_[512];                                                                                   \
            snprintf(rhiAssertBuf_,                                                                                    \
                     sizeof(rhiAssertBuf_),                                                                            \
                     "[RHI Assert] " format " (%s:%d)",                                                                \
                     ##__VA_ARGS__,                                                                                    \
                     __FILE__,                                                                                         \
                     __LINE__);                                                                                        \
            LOG_ERROR(rhiAssertBuf_);                                                                                  \
            assert(false);                                                                                             \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

#else

#define RHI_ASSERT(condition, message) ((void)0)
#define RHI_ASSERT_F(condition, format, ...) ((void)0)

#endif // NS_RHI_VALIDATION_ENABLED

/// リソース検証マクロ
#define RHI_ASSERT_RESOURCE_VALID(resource) RHI_ASSERT((resource) != nullptr, "Resource is null")
#define RHI_ASSERT_BUFFER_VALID(buffer) RHI_ASSERT((buffer) != nullptr, "Buffer is null or invalid")
#define RHI_ASSERT_TEXTURE_VALID(texture) RHI_ASSERT((texture) != nullptr, "Texture is null or invalid")
