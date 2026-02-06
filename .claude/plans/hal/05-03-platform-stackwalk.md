# 05-03: PlatformStackWalk（統合計画 → 細分化済み）

> **注意**: この計画は細分化されました。以下を参照してください：
> - [05-03a: スタックウォークインターフェース](05-03a-stackwalk-interface.md)
> - [05-03b: Windowsスタックウォーク実装](05-03b-windows-stackwalk.md)

---

# 05-03: PlatformStackWalk（旧）

## 目的

スタックトレース取得機能を実装する。

## 参考

[Platform_Abstraction_Layer_Part5.md](docs/Platform_Abstraction_Layer_Part5.md) - セクション4「スタックウォーク」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`int32`, `uint64`）

## 依存（HAL）

- 01-04 プラットフォーム型（`ANSICHAR`）

## 必要なヘッダ（Windows実装）

- `<windows.h>` - `RtlCaptureStackBackTrace`
- `<dbghelp.h>` - シンボル解決（`SymFromAddr`, `SymGetLineFromAddr64`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformStackWalk.h`
- `source/engine/hal/Public/Windows/WindowsPlatformStackWalk.h`
- `source/engine/hal/Public/HAL/PlatformStackWalk.h`
- `source/engine/hal/Private/Windows/WindowsPlatformStackWalk.cpp`

## TODO

- [ ] `ProgramCounterSymbolInfo` 構造体
- [ ] `GenericPlatformStackWalk` 構造体
- [ ] `WindowsPlatformStackWalk` 構造体（CaptureStackBackTrace使用）
- [ ] シンボル解決機能（オプション）

## 実装内容

```cpp
// GenericPlatformStackWalk.h
namespace NS
{
    struct ProgramCounterSymbolInfo
    {
        ANSICHAR moduleName[256];
        ANSICHAR functionName[256];
        ANSICHAR filename[256];
        int32 lineNumber;
        uint64 programCounter;
    };

    struct GenericPlatformStackWalk
    {
        static constexpr int32 kMaxStackDepth = 100;

        static int32 CaptureStackBackTrace(uint64* backTrace, int32 maxDepth, int32 skipCount = 0);

        static bool ProgramCounterToSymbolInfo(uint64 programCounter, ProgramCounterSymbolInfo& outInfo);

        static void InitStackWalking();
    };
}

// WindowsPlatformStackWalk.h
namespace NS
{
    struct WindowsPlatformStackWalk : public GenericPlatformStackWalk
    {
        static int32 CaptureStackBackTrace(uint64* backTrace, int32 maxDepth, int32 skipCount = 0)
        {
            return ::RtlCaptureStackBackTrace(skipCount, maxDepth, (void**)backTrace, nullptr);
        }
    };

    typedef WindowsPlatformStackWalk PlatformStackWalk;
}
```

## 検証

- スタックトレースが正しくキャプチャされる
- シンボル情報が解決される（デバッグビルド）
