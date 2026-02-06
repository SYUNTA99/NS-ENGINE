# 09-02a: Windows COM初期化

## 目的

Windows COMの初期化/終了機能を実装する。

## 参考

[Platform_Abstraction_Layer_Part9.md](docs/Platform_Abstraction_Layer_Part9.md) - セクション2「Windows固有機能」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint8`）

## 依存（HAL）

- 09-01b WindowsPlatformMisc（継承）

## 必要なヘッダ（Windows）

- `<objbase.h>` - `CoInitializeEx`, `CoUninitialize`

## 必要なライブラリ（Windows）

- `ole32.lib`

## 変更対象

変更:
- `source/engine/hal/Public/Windows/WindowsPlatformMisc.h`（メソッド追加）
- `source/engine/hal/Private/Windows/WindowsPlatformMisc.cpp`（実装追加）

## TODO

- [ ] `COMModel` 列挙型定義
- [ ] `CoInitialize` メソッド追加
- [ ] `CoUninitialize` メソッド追加
- [ ] スレッドセーフな初期化カウント管理

## 実装内容

### WindowsPlatformMisc.h（追加）

```cpp
// WindowsPlatformMisc.h に追加

namespace NS
{
    /// COMスレッディングモデル
    enum class COMModel : uint8
    {
        Singlethreaded = 0,  ///< シングルスレッドアパートメント (STA)
        Multithreaded = 1    ///< マルチスレッドアパートメント (MTA)
    };

    struct WindowsPlatformMisc : public GenericPlatformMisc
    {
        // ... 既存のメソッド ...

        /// COM初期化
        /// @param model スレッディングモデル（デフォルトはMTA）
        /// @note 各スレッドで呼び出す必要がある
        static void CoInitialize(COMModel model = COMModel::Multithreaded);

        /// COM終了
        /// @note CoInitializeと対になるように呼び出す
        static void CoUninitialize();

        /// COMが初期化済みかどうか（現在のスレッド）
        static bool IsCOMInitialized();
    };
}
```

### WindowsPlatformMisc.cpp（追加）

```cpp
#include <objbase.h>

#pragma comment(lib, "ole32.lib")

namespace NS
{
    // スレッドローカルな初期化カウント
    static thread_local int32 s_comInitCount = 0;

    void WindowsPlatformMisc::CoInitialize(COMModel model)
    {
        if (s_comInitCount == 0)
        {
            DWORD flags = (model == COMModel::Multithreaded)
                ? COINIT_MULTITHREADED
                : COINIT_APARTMENTTHREADED;

            HRESULT hr = ::CoInitializeEx(nullptr, flags);

            // S_OK または S_FALSE（既に初期化済み）は成功
            if (SUCCEEDED(hr))
            {
                ++s_comInitCount;
            }
        }
        else
        {
            ++s_comInitCount;
        }
    }

    void WindowsPlatformMisc::CoUninitialize()
    {
        if (s_comInitCount > 0)
        {
            --s_comInitCount;
            if (s_comInitCount == 0)
            {
                ::CoUninitialize();
            }
        }
    }

    bool WindowsPlatformMisc::IsCOMInitialized()
    {
        return s_comInitCount > 0;
    }
}
```

## 検証

- `CoInitialize()` / `CoUninitialize()` がペアで動作
- 複数回の `CoInitialize()` に対応（参照カウント）
- スレッドごとに独立した状態管理

## 注意事項

- COMはスレッドごとに初期化が必要
- MTA（マルチスレッド）がゲームエンジンでは一般的
- 初期化と終了は必ずペアで呼ぶ

## 次のサブ計画

→ 09-02b: Windowsレジストリ・バージョン
