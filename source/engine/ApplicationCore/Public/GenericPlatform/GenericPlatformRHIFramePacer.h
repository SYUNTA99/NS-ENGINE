/// @file GenericPlatformRHIFramePacer.h
/// @brief RHI フレームペーサー (VSync制御) (12-02)
#pragma once

#include <cstdint>

namespace NS
{
    /// フレームペーサー (VSync制御)
    class GenericPlatformRHIFramePacer
    {
    public:
        /// フレームペースがサポートされているか
        static bool SupportsFramePace(int32_t DesiredFrameRate)
        {
            if (MaxRefreshRate <= 0 || MaxSyncInterval <= 0)
            {
                return false;
            }
            return DesiredFrameRate > 0 && DesiredFrameRate <= MaxRefreshRate;
        }

        /// フレームペース設定 (ターゲットFPS)
        static void SetFramePace(int32_t DesiredFrameRate)
        {
            if (DesiredFrameRate > 0 && MaxRefreshRate > 0)
            {
                CurrentSyncInterval = MaxRefreshRate / DesiredFrameRate;
                if (CurrentSyncInterval < 1)
                    CurrentSyncInterval = 1;
                if (CurrentSyncInterval > MaxSyncInterval)
                    CurrentSyncInterval = MaxSyncInterval;
            }
        }

        /// 現在のフレームペース取得 (FPS)
        static int32_t GetFramePace()
        {
            if (CurrentSyncInterval > 0 && MaxRefreshRate > 0)
            {
                return MaxRefreshRate / CurrentSyncInterval;
            }
            return 0;
        }

        /// SyncInterval から FPS への変換
        static int32_t SyncIntervalToFramePace(int32_t SyncInterval)
        {
            if (SyncInterval > 0 && MaxRefreshRate > 0)
            {
                return MaxRefreshRate / SyncInterval;
            }
            return 0;
        }

        /// FPS から SyncInterval への変換
        static int32_t FramePaceToSyncInterval(int32_t FramePace)
        {
            if (FramePace > 0 && MaxRefreshRate > 0)
            {
                return MaxRefreshRate / FramePace;
            }
            return 0;
        }

        /// 最大リフレッシュレート (プラットフォームが設定)
        static inline int32_t MaxRefreshRate = 60;
        /// 最大SyncInterval (プラットフォームが設定)
        static inline int32_t MaxSyncInterval = 4;
        /// 現在のSyncInterval
        static inline int32_t CurrentSyncInterval = 1;
    };

} // namespace NS
