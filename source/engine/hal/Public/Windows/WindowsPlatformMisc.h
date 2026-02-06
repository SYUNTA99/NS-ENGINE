/// @file WindowsPlatformMisc.h
/// @brief Windows固有のプラットフォーム機能
#pragma once

#include "HAL/PlatformMisc.h"

namespace NS
{
    /// COMスレッディングモデル
    enum class COMModel : uint8
    {
        Singlethreaded = 0, ///< シングルスレッドアパートメント (STA)
        Multithreaded = 1   ///< マルチスレッドアパートメント (MTA)
    };

    /// ストレージデバイスタイプ
    enum class StorageDeviceType : uint8
    {
        Unknown = 0,
        HDD = 1,
        SSD = 2,
        NVMe = 3
    };

    /// Windows固有のプラットフォーム機能
    struct WindowsPlatformMisc : public GenericPlatformMisc
    {
        // =========================================================================
        // CPU情報
        // =========================================================================

        static void PlatformInit();
        static uint32 GetCPUInfo();
        static CPUInfo GetCPUDetails();
        static uint32 GetCacheLineSize();

#if PLATFORM_CPU_X86_FAMILY
        static uint32 GetFeatureBits_X86();
        static bool CheckFeatureBit_X86(uint32 featureBit);
        static bool HasAVX2InstructionSupport();
        static bool HasAVX512InstructionSupport();
#endif

        static const TCHAR* GetPlatformName();
        static const TCHAR* GetOSVersion();

        // =========================================================================
        // COM管理
        // =========================================================================

        /// COM初期化
        /// @param model スレッディングモデル（デフォルトはMTA）
        /// @note 各スレッドで呼び出す必要がある
        static void CoInitialize(COMModel model = COMModel::Multithreaded);

        /// COM終了
        /// @note CoInitializeと対になるように呼び出す
        static void CoUninitialize();

        /// COMが初期化済みかどうか（現在のスレッド）
        static bool IsCOMInitialized();

        // =========================================================================
        // レジストリ・バージョン
        // =========================================================================

        /// レジストリキーをクエリ
        /// @param key ルートキー（HKEY_LOCAL_MACHINE等）
        /// @param subKey サブキーパス
        /// @param valueName 値名
        /// @param outData 出力先バッファ
        /// @param outDataSize バッファサイズ（文字数）
        /// @return 成功した場合true
        static bool QueryRegKey(void* key, const TCHAR* subKey, const TCHAR* valueName, TCHAR* outData,
                                SIZE_T outDataSize);

        /// Windowsバージョン検証
        /// @param majorVersion メジャーバージョン（10等）
        /// @param minorVersion マイナーバージョン（0等）
        /// @param buildNumber ビルド番号（0は無視）
        /// @return 指定バージョン以上の場合true
        static bool VerifyWindowsVersion(uint32 majorVersion, uint32 minorVersion, uint32 buildNumber = 0);

        // =========================================================================
        // システム状態
        // =========================================================================

        /// ストレージデバイスタイプ取得
        /// @param path ドライブパス（"C:\\"等）
        static StorageDeviceType GetStorageDeviceType(const TCHAR* path);

        /// リモートデスクトップセッションかどうか
        static bool IsRemoteSession();

        /// スクリーンセーバー防止
        static void PreventScreenSaver();

    private:
        static uint32 s_cpuFeatures;
        static CPUInfo s_cpuInfo;
        static bool s_initialized;
    };

    typedef WindowsPlatformMisc PlatformMisc;
} // namespace NS
