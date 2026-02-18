/// @file GenericPlatformStackWalk.h
/// @brief プラットフォーム非依存のスタックウォークインターフェース
#pragma once

#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"
#include <cstring>

namespace NS
{
    /// シンボル名の最大長
    constexpr int32 kMaxSymbolNameLength = 1024;

    /// モジュール名の最大長
    constexpr int32 kMaxModuleNameLength = 260;

    /// ファイル名の最大長
    constexpr int32 kMaxFilenameLength = 260;

    /// プログラムカウンタのシンボル情報
    ///
    /// 各文字列フィールドは常にヌル終端される。
    /// 文字列が長すぎる場合は切り詰められる。
    struct ProgramCounterSymbolInfo
    {
        ANSICHAR moduleName[kMaxModuleNameLength]{};   ///< モジュール名（DLL/EXE）
        ANSICHAR functionName[kMaxSymbolNameLength]{}; ///< 関数名（マングル解除済み）
        ANSICHAR filename[kMaxFilenameLength]{};       ///< ソースファイル名
        int32 lineNumber{0};                            ///< 行番号（0 = 不明、-1 = シンボルなし）
        int32 columnNumber{0};                          ///< 列番号（0 = 不明）
        uint64 programCounter{0};                       ///< プログラムカウンタ値
        uint64 offsetInModule{0};                       ///< モジュール内オフセット

        /// デフォルト初期化
        ProgramCounterSymbolInfo()  
        {
            moduleName[0] = '\0';
            functionName[0] = '\0';
            filename[0] = '\0';
        }

        /// シンボルが解決されたか
        [[nodiscard]] bool IsResolved() const { return functionName[0] != '\0'; }

        /// ソース位置が利用可能か
        [[nodiscard]] bool HasSourceInfo() const { return lineNumber > 0; }
    };

    /// プラットフォーム非依存のスタックウォークインターフェース
    ///
    /// ## スレッドセーフティ
    ///
    /// - **InitStackWalking()**: メインスレッドから一度だけ呼び出す
    /// - **CaptureStackBackTrace()**: スレッドセーフ（自スレッドのスタックのみキャプチャ）
    /// - **ProgramCounterToSymbolInfo()**: スレッドセーフ（内部で同期）
    ///
    /// ## シンボル解決の前提条件
    ///
    /// - Windows: PDBファイルが実行ファイルと同じディレクトリに存在
    /// - シンボルサーバー設定（_NT_SYMBOL_PATH環境変数）
    ///
    /// ## パフォーマンス
    ///
    /// - CaptureStackBackTrace: 高速（μsオーダー）
    /// - ProgramCounterToSymbolInfo: 遅い（msオーダー、初回はさらに遅い）
    struct GenericPlatformStackWalk
    {
        /// 最大スタック深度
        static constexpr int32 kMaxStackDepth = 100;

        /// 推奨スタック深度（パフォーマンスバランス）
        static constexpr int32 kDefaultStackDepth = 32;

        /// スタックウォーク初期化
        ///
        /// @note メインスレッドから起動時に一度呼び出す
        /// @note 未初期化でも動作するが、シンボル解決が失敗する可能性
        static void InitStackWalking();

        /// 初期化済みかどうか
        static bool IsInitialized();

        /// スタックバックトレースをキャプチャ
        ///
        /// @param backTrace 出力先配列（nullptrでないこと）
        /// @param maxDepth 最大深度（1〜kMaxStackDepth）
        /// @param skipCount スキップするフレーム数（0以上）
        /// @return キャプチャしたフレーム数（0 = 失敗）
        ///
        /// @note 自スレッドのスタックのみキャプチャ可能
        static int32 CaptureStackBackTrace(uint64* backTrace, int32 maxDepth, int32 skipCount = 0);

        /// プログラムカウンタからシンボル情報を解決
        ///
        /// @param programCounter プログラムカウンタ値（0は無効）
        /// @param outInfo 出力先シンボル情報（初期化される）
        /// @return 解決成功した場合true（部分解決も含む）
        static bool ProgramCounterToSymbolInfo(uint64 programCounter, ProgramCounterSymbolInfo& outInfo);

        /// 複数のプログラムカウンタを一括解決
        ///
        /// @param programCounters プログラムカウンタ配列
        /// @param outInfos 出力先シンボル情報配列
        /// @param count 配列サイズ
        /// @return 解決成功した数
        static int32 ProgramCountersToSymbolInfos(const uint64* programCounters,
                                                  ProgramCounterSymbolInfo* outInfos,
                                                  int32 count);

    protected:
        static bool s_initialized;
    };

    /// 安全な文字列コピー（ヌル終端保証）
    inline void SafeStrCopy(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src)
    {
        if (destSize == 0)
        {
            return;
        }
        SIZE_T const srcLen = std::strlen(src);
        SIZE_T const copyLen = (srcLen < destSize - 1) ? srcLen : destSize - 1;
        std::memcpy(dest, src, copyLen);
        dest[copyLen] = '\0';
    }
} // namespace NS
