# 09-02b: Windowsレジストリ・バージョン

## 目的

Windowsレジストリクエリとバージョン検証機能を実装する。

## 参考

[Platform_Abstraction_Layer_Part9.md](docs/Platform_Abstraction_Layer_Part9.md) - セクション2「Windows固有機能」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`uint32`, `uint8`）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`, `SIZE_T`）
- 09-02a Windows COM初期化

## 必要なヘッダ（Windows）

- `<windows.h>` - `HKEY`, レジストリAPI, バージョンAPI
- `<versionhelpers.h>` - バージョンヘルパー（オプション）

## 必要なライブラリ（Windows）

- `advapi32.lib` - レジストリAPI

## 変更対象

変更:
- `source/engine/hal/Public/Windows/WindowsPlatformMisc.h`（メソッド追加）
- `source/engine/hal/Private/Windows/WindowsPlatformMisc.cpp`（実装追加）

## TODO

- [ ] `QueryRegKey` メソッド追加
- [ ] `VerifyWindowsVersion` メソッド追加
- [ ] `StorageDeviceType` 列挙型定義
- [ ] `GetStorageDeviceType` メソッド追加
- [ ] `IsRemoteSession` メソッド追加
- [ ] `PreventScreenSaver` メソッド追加

## 実装内容

### WindowsPlatformMisc.h（追加）

```cpp
// WindowsPlatformMisc.h に追加

namespace NS
{
    /// ストレージデバイスタイプ
    enum class StorageDeviceType : uint8
    {
        Unknown = 0,
        HDD = 1,
        SSD = 2,
        NVMe = 3
    };

    struct WindowsPlatformMisc : public GenericPlatformMisc
    {
        // ... 既存のメソッド ...

        /// レジストリキーをクエリ
        /// @param key ルートキー（HKEY_LOCAL_MACHINE等）
        /// @param subKey サブキーパス
        /// @param valueName 値名
        /// @param outData 出力先バッファ
        /// @param outDataSize バッファサイズ
        /// @return 成功した場合true
        static bool QueryRegKey(void* key, const TCHAR* subKey, const TCHAR* valueName,
                                TCHAR* outData, SIZE_T outDataSize);

        /// Windowsバージョン検証
        /// @param majorVersion メジャーバージョン（10等）
        /// @param minorVersion マイナーバージョン（0等）
        /// @param buildNumber ビルド番号（0は無視）
        /// @return 指定バージョン以上の場合true
        static bool VerifyWindowsVersion(uint32 majorVersion, uint32 minorVersion, uint32 buildNumber = 0);

        /// ストレージデバイスタイプ取得
        /// @param path ドライブパス（"C:\\"等）
        static StorageDeviceType GetStorageDeviceType(const TCHAR* path);

        /// リモートデスクトップセッションかどうか
        static bool IsRemoteSession();

        /// スクリーンセーバー防止
        static void PreventScreenSaver();
    };
}
```

### WindowsPlatformMisc.cpp（追加）

```cpp
#include <windows.h>
#include <winioctl.h>  // IOCTL_STORAGE_QUERY_PROPERTY

namespace NS
{
    bool WindowsPlatformMisc::QueryRegKey(void* key, const TCHAR* subKey, const TCHAR* valueName,
                                          TCHAR* outData, SIZE_T outDataSize)
    {
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(static_cast<HKEY>(key), subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        {
            return false;
        }

        DWORD type = 0;
        DWORD size = static_cast<DWORD>(outDataSize * sizeof(TCHAR));
        bool success = (RegQueryValueExW(hKey, valueName, nullptr, &type,
                        reinterpret_cast<LPBYTE>(outData), &size) == ERROR_SUCCESS);

        RegCloseKey(hKey);
        return success && (type == REG_SZ || type == REG_EXPAND_SZ);
    }

    bool WindowsPlatformMisc::VerifyWindowsVersion(uint32 majorVersion, uint32 minorVersion, uint32 buildNumber)
    {
        OSVERSIONINFOEXW osvi = {};
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        osvi.dwMajorVersion = majorVersion;
        osvi.dwMinorVersion = minorVersion;
        osvi.dwBuildNumber = buildNumber;

        DWORDLONG conditionMask = 0;
        VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

        DWORD typeMask = VER_MAJORVERSION | VER_MINORVERSION;
        if (buildNumber > 0)
        {
            VER_SET_CONDITION(conditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
            typeMask |= VER_BUILDNUMBER;
        }

        return VerifyVersionInfoW(&osvi, typeMask, conditionMask) != 0;
    }

    StorageDeviceType WindowsPlatformMisc::GetStorageDeviceType(const TCHAR* path)
    {
        // ドライブ文字を抽出
        TCHAR rootPath[4] = {};
        rootPath[0] = path[0];
        rootPath[1] = TEXT(':');
        rootPath[2] = TEXT('\\');
        rootPath[3] = TEXT('\0');

        // ドライブタイプを取得
        UINT driveType = GetDriveTypeW(rootPath);
        if (driveType != DRIVE_FIXED)
        {
            return StorageDeviceType::Unknown;
        }

        // 詳細な検出にはDeviceIoControlが必要（簡易実装ではHDDとする）
        // TODO: IOCTL_STORAGE_QUERY_PROPERTY でSSD/NVMe検出
        return StorageDeviceType::HDD;
    }

    bool WindowsPlatformMisc::IsRemoteSession()
    {
        return GetSystemMetrics(SM_REMOTESESSION) != 0;
    }

    void WindowsPlatformMisc::PreventScreenSaver()
    {
        SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
    }
}
```

## 検証

- `QueryRegKey` がレジストリ値を取得できる
- `VerifyWindowsVersion(10, 0)` がWindows 10以降でtrue
- `IsRemoteSession()` がリモートデスクトップ時にtrue

## 注意事項

- レジストリアクセスには適切な権限が必要
- `VerifyVersionInfoW` はWindows 8.1以降でマニフェストが必要
- SSD/NVMe検出は `DeviceIoControl` が必要（高度な実装）

## 次のサブ計画

→ 10-01a: TCharテンプレート
