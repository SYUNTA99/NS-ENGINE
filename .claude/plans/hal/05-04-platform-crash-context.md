# 05-04: PlatformCrashContext（統合計画 → 細分化済み）

> **注意**: この計画は細分化されました。以下を参照してください：
> - [05-04a: クラッシュコンテキストインターフェース](05-04a-crashcontext-interface.md)
> - [05-04b: Windowsクラッシュコンテキスト実装](05-04b-windows-crashcontext.md)

---

# 05-04: PlatformCrashContext（旧）

## 目的

クラッシュコンテキスト収集機能を実装する。

## 参考

[Platform_Abstraction_Layer_Part5.md](docs/Platform_Abstraction_Layer_Part5.md) - セクション3「クラッシュ処理」

## 依存（commonモジュール）

（commonモジュールへの直接依存なし）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformCrashContext.h`
- `source/engine/hal/Public/Windows/WindowsPlatformCrashContext.h`
- `source/engine/hal/Public/HAL/PlatformCrashContext.h`
- `source/engine/hal/Private/Windows/WindowsPlatformCrashContext.cpp`

## TODO

- [ ] `CrashContextType` 列挙型
- [ ] `GenericPlatformCrashContext` クラス
- [ ] `WindowsPlatformCrashContext` クラス
- [ ] クラッシュハンドラ登録

## 実装内容

```cpp
// GenericPlatformCrashContext.h
namespace NS
{
    enum class CrashContextType
    {
        Crash,
        Assert,
        Ensure,
        Stall,
        GPUCrash,
        Hang,
        OutOfMemory,
        AbnormalShutdown
    };

    class GenericPlatformCrashContext
    {
    public:
        GenericPlatformCrashContext(CrashContextType type);
        virtual ~GenericPlatformCrashContext() = default;

        CrashContextType GetType() const { return m_type; }

        virtual void CaptureContext();
        virtual void SetErrorMessage(const TCHAR* message);

        static void SetEngineVersion(const TCHAR* version);
        static void SetCrashHandler(void(*handler)(void*));

    protected:
        CrashContextType m_type;
        TCHAR m_errorMessage[1024];
    };
}

// WindowsPlatformCrashContext.h
namespace NS
{
    class WindowsPlatformCrashContext : public GenericPlatformCrashContext
    {
    public:
        using GenericPlatformCrashContext::GenericPlatformCrashContext;

        void CaptureContext() override;

        static void SetUnhandledExceptionFilter();
    };

    typedef WindowsPlatformCrashContext PlatformCrashContext;
}
```

## 検証

- クラッシュ時にコンテキストが収集される
- エラーメッセージが保存される
