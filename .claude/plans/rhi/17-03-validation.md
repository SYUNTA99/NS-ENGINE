# 17-03: 検証レイヤー

## 目的

RHI操作の検証とデバッグ機能を定義する。

## 参照ドキュメント

- 17-01-rhi-result.md (RHICheck.h)
- 09-03-gpu-event.md (デバッグマーカー)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIValidation.h`
- `Source/Engine/RHI/Private/RHIValidation.cpp`

## TODO

### 1. 検証レベル

```cpp
#pragma once

#include "RHICheck.h"

namespace NS::RHI
{
    /// 検証レベル
    enum class ERHIValidationLevel : uint8
    {
        /// 無効
        Disabled,

        /// 基本（致命的なエラーのみ）。
        Basic,

        /// 標準（一般的なエラー）。
        Standard,

        /// 詳細（パフォーマンス警告含む）。
        Verbose,

        /// 最大（すべての検証）。
        Maximum,
    };

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
}
```

- [ ] ERHIValidationLevel 列挙型
- [ ] ERHIValidationCategory 列挙型

### 2. 検証メッセージ

```cpp
namespace NS::RHI
{
    /// 検証メッセージ重要度
    enum class ERHIValidationSeverity : uint8
    {
        Info,
        Warning,
        Error,
        Corruption,  // データ破損の可能性
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
    using RHIValidationCallback = void(*)(
        const RHIValidationMessage& message,
        void* userData);
}
```

- [ ] ERHIValidationSeverity 列挙型
- [ ] RHIValidationMessage 構造体

### 3. 検証レイヤー設定

```cpp
namespace NS::RHI
{
    /// 検証レイヤー設定
    struct RHI_API RHIValidationConfig
    {
        /// 検証レベル
        ERHIValidationLevel level = ERHIValidationLevel::Disabled;

        /// 有効なカテゴリ
        ERHIValidationCategory enabledCategories = ERHIValidationCategory::All;

        /// GPUベース検証（パフォーマンス影響大）。
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

        /// デバッグビルド向か
        static RHIValidationConfig Debug() {
            RHIValidationConfig config;
            config.level = ERHIValidationLevel::Standard;
            config.gpuBasedValidation = false;
            config.shaderDebugInfo = true;
            config.breakOnError = true;
            return config;
        }

        /// 開発ビルド向か
        static RHIValidationConfig Development() {
            RHIValidationConfig config;
            config.level = ERHIValidationLevel::Basic;
            config.shaderDebugInfo = true;
            return config;
        }

        /// リリースビルド向か
        static RHIValidationConfig Release() {
            RHIValidationConfig config;
            config.level = ERHIValidationLevel::Disabled;
            return config;
        }

        /// 最大検証（問題調査用）。
        static RHIValidationConfig Maximum() {
            RHIValidationConfig config;
            config.level = ERHIValidationLevel::Maximum;
            config.gpuBasedValidation = true;
            config.shaderDebugInfo = true;
            config.breakOnError = true;
            return config;
        }
    };
}
```

- [ ] RHIValidationConfig 構造体
- [ ] プリセット

### 4. 検証レイヤーインターフェース

```cpp
namespace NS::RHI
{
    /// 検証レイヤー（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// 検証設定
        virtual void ConfigureValidation(const RHIValidationConfig& config) = 0;

        /// 検証レベル取得
        virtual ERHIValidationLevel GetValidationLevel() const = 0;

        /// 検証が有効か
        bool IsValidationEnabled() const {
            return GetValidationLevel() != ERHIValidationLevel::Disabled;
        }

        /// メッセージ抑制追加
        virtual void SuppressValidationMessage(uint32 messageId) = 0;

        /// メッセージ抑制解除
        virtual void UnsuppressValidationMessage(uint32 messageId) = 0;

        /// 検証統計取得
        struct ValidationStats {
            uint32 errorCount;
            uint32 warningCount;
            uint32 infoCount;
            uint32 suppressedCount;
        };
        virtual ValidationStats GetValidationStats() const = 0;

        /// 検証統計リセット
        virtual void ResetValidationStats() = 0;
    };
}
```

- [ ] ConfigureValidation
- [ ] ValidationStats

### 5. デバッグオブジェクト命合

```cpp
namespace NS::RHI
{
    /// デバッグ名設定（RHIResourceに追加）。
    class IRHIResource
    {
    public:
        /// デバッグ名設定
        virtual void SetDebugName(const char* name) = 0;

        /// デバッグ名取得
        virtual const char* GetDebugName() const = 0;
    };

    /// デバッグマーカー設定（グローバルヘルパー）。
    namespace RHIDebug
    {
        /// リソースにデバッグ名設定
        template<typename T>
        T* SetName(T* resource, const char* name) {
            if (resource) {
                resource->SetDebugName(name);
            }
            return resource;
        }

        /// フォーマット付きデバッグ名設定
        template<typename T>
        T* SetNameF(T* resource, const char* format, ...) {
            if (resource) {
                char buffer[256];
                va_list args;
                va_start(args, format);
                vsnprintf(buffer, sizeof(buffer), format, args);
                va_end(args);
                resource->SetDebugName(buffer);
            }
            return resource;
        }
    }
}
```

- [ ] SetDebugName / GetDebugName
- [ ] RHIDebug ヘルパー

### 6. RHIアサーチ

```cpp
namespace NS::RHI
{
    /// RHIアサートのクロ
    #if NS_RHI_VALIDATION_ENABLED

    #define RHI_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                NS_LOG_ERROR("[RHI Assert] %s", message); \
                NS_DEBUG_BREAK(); \
            } \
        } while(0)

    #define RHI_ASSERT_F(condition, format, ...) \
        do { \
            if (!(condition)) { \
                NS_LOG_ERROR("[RHI Assert] " format, __VA_ARGS__); \
                NS_DEBUG_BREAK(); \
            } \
        } while(0)

    #define RHI_VERIFY(condition, message) RHI_ASSERT(condition, message)

    #else

    #define RHI_ASSERT(condition, message) ((void)0)
    #define RHI_ASSERT_F(condition, format, ...) ((void)0)
    #define RHI_VERIFY(condition, message) ((void)(condition))

    #endif

    /// リソース検証マクロ
    #define RHI_ASSERT_RESOURCE_VALID(resource) \
        RHI_ASSERT((resource) != nullptr, "Resource is null")

    #define RHI_ASSERT_BUFFER_VALID(buffer) \
        RHI_ASSERT((buffer) != nullptr && (buffer)->IsValid(), "Buffer is invalid")

    #define RHI_ASSERT_TEXTURE_VALID(texture) \
        RHI_ASSERT((texture) != nullptr && (texture)->IsValid(), "Texture is invalid")
}
```

- [ ] RHIアサートのクロ
- [ ] リソース検証マクロ

## 検証方法

- [ ] 検証レベルの動作
- [ ] メッセージ出力
- [ ] デバッグ名設定
- [ ] アサート動作
