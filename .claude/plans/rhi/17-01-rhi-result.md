# 17-01: RHI エラー処理ユーティリティ

## 目的

RHI操作のチェックマクロとプラットフォームエラーチェックを定義する。

## 参照ドキュメント

- NS::Result型（既存のエンジン基盤）。
- HAL設計原則

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHICheck.h`

## TODO

### 1. チェックマクロ

```cpp
// Source/Engine/RHI/Public/RHICheck.h

#pragma once
#include "RHITypes.h"

namespace NS::RHI
{
    //=================================================================
    // チェックマクロ（デバッグビルドのみ）
    //=================================================================

    #if NS_RHI_VALIDATION_ENABLED

    /// 条件チェック（falseならログ+ブレーク）
    #define RHI_CHECK(condition) \
        do { \
            if (!(condition)) { \
                NS_LOG_ERROR("[RHI] Check failed: %s (%s:%d)", \
                    #condition, __FILE__, __LINE__); \
                NS_DEBUG_BREAK(); \
            } \
        } while(0)

    /// 条件チェック（falseならログ+ブレーク、リリースでも評価）
    #define RHI_ENSURE(condition) \
        ([&]() -> bool { \
            bool _result = (condition); \
            if (!_result) { \
                NS_LOG_ERROR("[RHI] Ensure failed: %s (%s:%d)", \
                    #condition, __FILE__, __LINE__); \
                NS_DEBUG_BREAK(); \
            } \
            return _result; \
        }())

    #else

    #define RHI_CHECK(condition) ((void)0)
    #define RHI_ENSURE(condition) (!!(condition))

    #endif

    //=================================================================
    // HRESULT チェック（D3D12バックエンド内部用）
    //=================================================================

    /// HRESULT成功チェック（失敗時ログ+false返却）
    #define RHI_CHECK_HRESULT(hr) \
        ([&]() -> bool { \
            HRESULT _hr = (hr); \
            if (FAILED(_hr)) { \
                NS_LOG_ERROR("[RHI/D3D12] HRESULT failed: 0x%08X (%s:%d)", \
                    _hr, __FILE__, __LINE__); \
                return false; \
            } \
            return true; \
        }())

    //=================================================================
    // VkResult チェック（Vulkanバックエンド内部用）
    //=================================================================

    /// VkResult成功チェック（失敗時ログ+false返却）
    #define RHI_CHECK_VK(vkResult) \
        ([&]() -> bool { \
            auto _vk = (vkResult); \
            if (_vk != VK_SUCCESS) { \
                NS_LOG_ERROR("[RHI/Vulkan] VkResult failed: %d (%s:%d)", \
                    _vk, __FILE__, __LINE__); \
                return false; \
            } \
            return true; \
        }())

} // namespace NS::RHI
```

- [ ] `RHI_CHECK` マクロ
- [ ] `RHI_ENSURE` マクロ
- [ ] `RHI_CHECK_HRESULT` マクロ（D3D12バックエンド Private 用）
- [ ] `RHI_CHECK_VK` マクロ（Vulkan バックエンド Private 用）

### 2. リソース作成パターン

```cpp
// リソース作成: nullptrで失敗を表現
TRefCountPtr<IRHIBuffer> buffer(
    device->CreateBuffer(desc, "MyBuffer"));
if (!buffer) {
    // 失敗原因はCreateBuffer内部でログ出力済み
    return;
}

// 操作: boolまたはvoid
if (!swapChain->Present()) {
    // DeviceLostHandlerが自動的に呼ばれる
}
```

## 検証方法

- [ ] チェックマクロの動作
- [ ] プラットフォームエラーチェック
