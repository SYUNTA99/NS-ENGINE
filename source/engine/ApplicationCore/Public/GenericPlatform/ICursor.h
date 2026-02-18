/// @file ICursor.h
/// @brief カーソルインターフェース
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"
#include "ApplicationCore/InputTypes.h"
#include <algorithm>
#include <cstdint>
#include <string>

namespace NS
{
    /// カーソル抽象インターフェース
    class ICursor
    {
    public:
        ICursor() = default;
        virtual ~ICursor() = default;
        NS_DISALLOW_COPY_AND_MOVE(ICursor);

    public:
        // =================================================================
        // 型・サイズ
        // =================================================================
        [[nodiscard]] virtual MouseCursor::Type GetType() const = 0;
        virtual void SetType(MouseCursor::Type inType) = 0;
        virtual void GetSize(int32_t& width, int32_t& height) const = 0;

        // =================================================================
        // 位置
        // =================================================================
        virtual void GetPosition(Vector2D& outPosition) const = 0;
        virtual void SetPosition(int32_t x, int32_t y) = 0;

        // =================================================================
        // 表示・ロック
        // =================================================================
        virtual void Show(bool bShow) = 0;
        virtual void Lock(const PlatformRect* bounds) = 0;

        // =================================================================
        // カーソル形状オーバーライド
        // =================================================================

        /// 指定カーソル種別の形状をプラットフォーム固有ハンドルでオーバーライド
        virtual void SetTypeShape(MouseCursor::Type inCursorType, void* inCursorHandle)
        {
            (void)inCursorType;
            (void)inCursorHandle;
        }

        // =================================================================
        // カスタムカーソル作成
        // =================================================================

        /// ファイル(.ani/.cur)からカーソルを生成
        /// @return プラットフォーム固有カーソルハンドル (失敗時 nullptr)
        virtual void* CreateCursorFromFile(const std::wstring& inPath, Vector2D inHotSpot)
        {
            (void)inPath;
            (void)inHotSpot;
            return nullptr;
        }

        /// RGBAバッファからのカーソル生成がサポートされるか
        [[nodiscard]] virtual bool IsCreateCursorFromRGBABufferSupported() const { return false; }

        /// RGBAピクセルデータからカーソルを生成
        /// @param InPixels RGBA 8bpp ピクセルデータ
        /// @param InWidth 幅
        /// @param InHeight 高さ
        /// @param InHotSpot ホットスポット (0.0-1.0 正規化座標)
        /// @return プラットフォーム固有カーソルハンドル (失敗時 nullptr)
        virtual void* CreateCursorFromRGBABuffer(const uint8_t* inPixels,
                                                 int32_t inWidth,
                                                 int32_t inHeight,
                                                 Vector2D inHotSpot)
        {
            (void)inPixels;
            (void)inWidth;
            (void)inHeight;
            (void)inHotSpot;
            return nullptr;
        }

        // =================================================================
        // マウス加速度
        // =================================================================

        /// マウスデルタに対して2次加速度を適用
        /// @param Delta 入力デルタ（変更なしで返ることもある）
        /// @param Sensitivity 感度係数
        /// @return 加速度が適用されたデルタ
        static float CalculateDeltaWithAcceleration(float delta, float sensitivity = 1.0F)
        {
            constexpr float kNominalMovement = 20.0F;
            float const absDelta = delta < 0.0F ? -delta : delta;

            // 2次加速度: (delta^2 / nominal) * sign * sensitivity
            float accelerated = (absDelta * absDelta) / kNominalMovement;
            accelerated = std::max(accelerated, absDelta);

            float const sign = delta < 0.0F ? -1.0F : 1.0F;
            return sign * accelerated * sensitivity;
        }
    };

} // namespace NS
