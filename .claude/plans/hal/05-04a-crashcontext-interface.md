# 05-04a: クラッシュコンテキストインターフェース

## 目的

クラッシュコンテキスト収集のインターフェースを定義する。

## 参考

[Platform_Abstraction_Layer_Part5.md](docs/Platform_Abstraction_Layer_Part5.md) - セクション3「クラッシュ処理」

## 依存（commonモジュール）

（commonモジュールへの直接依存なし）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformCrashContext.h`

## TODO

- [ ] `CrashContextType` 列挙型定義
- [ ] `GenericPlatformCrashContext` クラス定義
- [ ] クラッシュハンドラ型定義

## 実装内容

```cpp
// GenericPlatformCrashContext.h
#pragma once

#include "HAL/PlatformTypes.h"

namespace NS
{
    /// クラッシュコンテキストの種類
    enum class CrashContextType
    {
        Crash,           ///< 一般的なクラッシュ
        Assert,          ///< アサーション失敗
        Ensure,          ///< Ensure失敗（継続可能）
        Stall,           ///< ストール（長時間ブロック）
        GPUCrash,        ///< GPUクラッシュ
        Hang,            ///< ハング（応答なし）
        OutOfMemory,     ///< メモリ不足
        AbnormalShutdown ///< 異常終了
    };

    /// クラッシュハンドラ関数型
    using CrashHandlerFunc = void(*)(void* exceptionInfo);

    /// プラットフォーム非依存のクラッシュコンテキストクラス
    class GenericPlatformCrashContext
    {
    public:
        explicit GenericPlatformCrashContext(CrashContextType type);
        virtual ~GenericPlatformCrashContext() = default;

        /// コンテキスト種類を取得
        CrashContextType GetType() const { return m_type; }

        /// コンテキスト情報をキャプチャ
        virtual void CaptureContext();

        /// エラーメッセージを設定
        virtual void SetErrorMessage(const TCHAR* message);

        /// エラーメッセージを取得
        const TCHAR* GetErrorMessage() const { return m_errorMessage; }

        // 静的メソッド
        static void SetEngineVersion(const TCHAR* version);
        static void SetCrashHandler(CrashHandlerFunc handler);
        static CrashHandlerFunc GetCrashHandler();

    protected:
        CrashContextType m_type;
        TCHAR m_errorMessage[1024];

        static TCHAR s_engineVersion[64];
        static CrashHandlerFunc s_crashHandler;
    };
}
```

## 検証

- ヘッダがコンパイル可能
- `CrashContextType` の各値が使用可能

## 次のサブ計画

→ 05-04b: Windowsクラッシュコンテキスト実装
