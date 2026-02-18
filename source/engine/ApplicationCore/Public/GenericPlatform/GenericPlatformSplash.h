/// @file GenericPlatformSplash.h
/// @brief スプラッシュスクリーンインターフェース (12-01)
#pragma once

#include "common/utility/macros.h"
#include <cstdint>
#include <string>

namespace NS
{
    /// スプラッシュスクリーンテキストタイプ
    enum class SplashTextType : uint8_t
    {
        /// スタートアップ進捗テキスト
        StartupProgress,
        /// バージョン情報テキスト
        VersionInfo,
        /// ゲーム名テキスト
        GameName,
        /// 著作権テキスト
        CopyrightInfo
    };

    /// スプラッシュスクリーン汎用インターフェース
    class GenericPlatformSplash
    {
    public:
        GenericPlatformSplash() = default;
        virtual ~GenericPlatformSplash() = default;
        NS_DISALLOW_COPY_AND_MOVE(GenericPlatformSplash);

    public:
        /// スプラッシュ表示
        virtual void Show() {}

        /// スプラッシュ非表示
        virtual void Hide() {}

        /// 表示中か
        [[nodiscard]] virtual bool IsShown() const { return false; }

        /// テキスト設定
        virtual void SetSplashText(SplashTextType inType, const wchar_t* inText)
        {
            (void)inType;
            (void)inText;
        }

        /// 進捗設定 (0.0 - 1.0)
        virtual void SetProgress(float inProgress) { (void)inProgress; }

        /// カスタム画像設定
        virtual void SetCustomSplashImage(const wchar_t* inImagePath) { (void)inImagePath; }

        /// シングルトン
        static GenericPlatformSplash& Get();
    };

} // namespace NS
