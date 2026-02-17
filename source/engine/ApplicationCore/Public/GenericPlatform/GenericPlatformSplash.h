/// @file GenericPlatformSplash.h
/// @brief スプラッシュスクリーンインターフェース (12-01)
#pragma once

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
        virtual ~GenericPlatformSplash() = default;

        /// スプラッシュ表示
        virtual void Show() {}

        /// スプラッシュ非表示
        virtual void Hide() {}

        /// 表示中か
        virtual bool IsShown() const { return false; }

        /// テキスト設定
        virtual void SetSplashText(SplashTextType InType, const wchar_t* InText)
        {
            (void)InType;
            (void)InText;
        }

        /// 進捗設定 (0.0 - 1.0)
        virtual void SetProgress(float InProgress) { (void)InProgress; }

        /// カスタム画像設定
        virtual void SetCustomSplashImage(const wchar_t* InImagePath) { (void)InImagePath; }

        /// シングルトン
        static GenericPlatformSplash& Get();
    };

} // namespace NS
