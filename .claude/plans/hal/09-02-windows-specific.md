> **⚠️ SUPERSEDED**: この計画は細分化されました。
> - [09-02a: Windows COM初期化](09-02a-windows-com.md)
> - [09-02b: Windowsレジストリ・バージョン](09-02b-windows-registry-version.md)

# 09-02: Windows固有機能（旧版）

## 目的

Windows固有のプラットフォーム機能を実装する。

## 参考

[Platform_Abstraction_Layer_Part9.md](docs/Platform_Abstraction_Layer_Part9.md) - セクション2「Windows固有機能」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint8`, `uint32`）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`, `SIZE_T`）
- 09-01 CPU機能検出（`GenericPlatformMisc`）

## 必要なヘッダ（Windows実装）

- `<windows.h>` - `HKEY`, レジストリAPI, バージョンAPI
- `<objbase.h>` - `CoInitializeEx`, `CoUninitialize`

## 変更対象

変更:
- `source/engine/hal/Public/Windows/WindowsPlatformMisc.h`
- `source/engine/hal/Private/Windows/WindowsPlatformMisc.cpp`

## TODO

- [ ] COM初期化/終了
- [ ] レジストリクエリ
- [ ] Windowsバージョン検証
- [ ] ストレージデバイスタイプ検出

## 実装内容

```cpp
// WindowsPlatformMisc.h に追加

namespace NS
{
    enum class COMModel : uint8
    {
        Singlethreaded = 0,
        Multithreaded = 1
    };

    enum class StorageDeviceType : uint8
    {
        Unknown = 0,
        HDD = 1,
        SSD = 2,
        NVMe = 3
    };

    struct WindowsPlatformMisc : public GenericPlatformMisc
    {
        // COM
        static void CoInitialize(COMModel model = COMModel::Multithreaded);
        static void CoUninitialize();

        // レジストリ
        static bool QueryRegKey(HKEY key, const TCHAR* subKey, const TCHAR* valueName, TCHAR* outData, SIZE_T outDataSize);

        // バージョン
        static bool VerifyWindowsVersion(uint32 majorVersion, uint32 minorVersion, uint32 buildNumber = 0);

        // ストレージ
        static StorageDeviceType GetStorageDeviceType(const TCHAR* path);

        // その他
        static bool IsRemoteSession();
        static void PreventScreenSaver();
    };
}
```

## 検証

- COM初期化が正常動作
- Windowsバージョン検証が正しく判定
- レジストリクエリが値を取得できる

## 必要なヘッダ（Windows）

- `<windows.h>` - `HKEY`, レジストリAPI
- `<objbase.h>` - `CoInitializeEx`, `CoUninitialize`
- `<versionhelpers.h>` - バージョンヘルパー（オプション）

## 必要なライブラリ（Windows）

- `ole32.lib` - COM

## 注意事項

- COM初期化はスレッドごとに必要
- `COMModel::Multithreaded` がゲームエンジンでは一般的
- レジストリアクセスは権限に注意

## 次のサブ計画

→ 10-01: 文字列ユーティリティ
