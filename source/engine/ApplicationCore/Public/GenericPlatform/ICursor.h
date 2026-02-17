/// @file ICursor.h
/// @brief カーソルインターフェース
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"
#include "ApplicationCore/InputTypes.h"
#include <cstdint>
#include <string>

namespace NS
{
    /// カーソル抽象インターフェース
    class ICursor
    {
    public:
        virtual ~ICursor() = default;

        // =================================================================
        // 型・サイズ
        // =================================================================
        virtual MouseCursor::Type GetType() const = 0;
        virtual void SetType(MouseCursor::Type InType) = 0;
        virtual void GetSize(int32_t& Width, int32_t& Height) const = 0;

        // =================================================================
        // 位置
        // =================================================================
        virtual void GetPosition(Vector2D& OutPosition) const = 0;
        virtual void SetPosition(int32_t X, int32_t Y) = 0;

        // =================================================================
        // 表示・ロック
        // =================================================================
        virtual void Show(bool bShow) = 0;
        virtual void Lock(const PlatformRect* Bounds) = 0;

        // =================================================================
        // カーソル形状オーバーライド
        // =================================================================

        /// 指定カーソル種別の形状をプラットフォーム固有ハンドルでオーバーライド
        virtual void SetTypeShape(MouseCursor::Type InCursorType, void* InCursorHandle) { (void)InCursorType; (void)InCursorHandle; }

        // =================================================================
        // カスタムカーソル作成
        // =================================================================

        /// ファイル(.ani/.cur)からカーソルを生成
        /// @return プラットフォーム固有カーソルハンドル (失敗時 nullptr)
        virtual void* CreateCursorFromFile(const std::wstring& InPath, Vector2D InHotSpot)
        {
            (void)InPath;
            (void)InHotSpot;
            return nullptr;
        }

        /// RGBAバッファからのカーソル生成がサポートされるか
        virtual bool IsCreateCursorFromRGBABufferSupported() const { return false; }

        /// RGBAピクセルデータからカーソルを生成
        /// @param InPixels RGBA 8bpp ピクセルデータ
        /// @param InWidth 幅
        /// @param InHeight 高さ
        /// @param InHotSpot ホットスポット (0.0-1.0 正規化座標)
        /// @return プラットフォーム固有カーソルハンドル (失敗時 nullptr)
        virtual void* CreateCursorFromRGBABuffer(const uint8_t* InPixels, int32_t InWidth, int32_t InHeight, Vector2D InHotSpot)
        {
            (void)InPixels;
            (void)InWidth;
            (void)InHeight;
            (void)InHotSpot;
            return nullptr;
        }

        // =================================================================
        // マウス加速度
        // =================================================================

        /// マウスデルタに対して2次加速度を適用
        /// @param Delta 入力デルタ（変更なしで返ることもある）
        /// @param Sensitivity 感度係数
        /// @return 加速度が適用されたデルタ
        static float CalculateDeltaWithAcceleration(float Delta, float Sensitivity = 1.0f)
        {
            constexpr float NominalMovement = 20.0f;
            float absDelta = Delta < 0.0f ? -Delta : Delta;

            // 2次加速度: (delta^2 / nominal) * sign * sensitivity
            float accelerated = (absDelta * absDelta) / NominalMovement;
            if (accelerated < absDelta)
                accelerated = absDelta; // 最低でも元のデルタ以上

            float sign = Delta < 0.0f ? -1.0f : 1.0f;
            return sign * accelerated * Sensitivity;
        }
    };

} // namespace NS
