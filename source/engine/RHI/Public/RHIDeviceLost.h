/// @file RHIDeviceLost.h
/// @brief デバイスロスト検出・通知・回復
/// @details デバイスロスト理由、詳細情報、コールバックハンドラー、
///          デバイス回復マネージャーを提供。
/// @see 17-02-device-lost.md
#pragma once

#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // ERHIDeviceLostReason (17-02)
    //=========================================================================

    /// デバイスロスト理由
    enum class ERHIDeviceLostReason : uint8
    {
        /// 不明
        Unknown,

        /// GPUハング（タイムアウト）
        Hung,

        /// GPUリセット
        Reset,

        /// ドライバのアップグレード
        DriverUpgrade,

        /// ドライバ内部エラー
        DriverInternalError,

        /// 無効なGPUコマンド
        InvalidGPUCommand,

        /// GPUページフォールト
        PageFault,

        /// 電力イベント
        PowerEvent,

        /// 物理的な削除
        PhysicalRemoval,

        /// メモリ不足
        OutOfMemory,
    };

    /// デバイスロスト理由名取得
    RHI_API const char* GetDeviceLostReasonName(ERHIDeviceLostReason reason);

    //=========================================================================
    // RHIDeviceLostInfo (17-02)
    //=========================================================================

    /// デバイスロスト情報
    struct RHI_API RHIDeviceLostInfo
    {
        /// ロスト理由
        ERHIDeviceLostReason reason = ERHIDeviceLostReason::Unknown;

        /// ネイティブエラーコード（HRESULT等）
        int32 nativeErrorCode = 0;

        /// 詳細メッセージ
        char message[512] = {};

        /// 最後に実行されたGPUコマンド（可能な場合）
        char lastGPUCommand[256] = {};

        /// ブレッドクラム情報（利用可能な場合）
        uint32 lastBreadcrumbId = 0;
        char lastBreadcrumbMessage[256] = {};

        /// フォールトアドレス（ページフォールト時）
        uint64 faultAddress = 0;

        /// タイムスタンプ
        uint64 timestamp = 0;
    };

    //=========================================================================
    // RHIDeviceLostHandler (17-02)
    //=========================================================================

    /// デバイスロストコールバック（DeviceLostHandler用）
    using RHIDeviceLostHandlerCallback = void (*)(IRHIDevice* device, const RHIDeviceLostInfo& info, void* userData);

    /// デバイスロストハンドラー
    class RHI_API RHIDeviceLostHandler
    {
    public:
        RHIDeviceLostHandler() = default;

        /// 初期化
        void Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // コールバック登録
        //=====================================================================

        /// コールバック追加
        void AddCallback(RHIDeviceLostHandlerCallback callback, void* userData = nullptr);

        /// コールバック削除
        void RemoveCallback(RHIDeviceLostHandlerCallback callback);

        //=====================================================================
        // ポーリング
        //=====================================================================

        /// デバイスロストをポーリング
        /// ロストしていればコールバックを呼び出す
        /// @return デバイスがロストしたか
        bool Poll();

        /// 自動ポーリング有効化（別スレッド）
        void EnableAutoPolling(uint32 intervalMs = 100);

        /// 自動ポーリング無効化
        void DisableAutoPolling();

    private:
        IRHIDevice* m_device = nullptr;

        struct CallbackEntry
        {
            RHIDeviceLostHandlerCallback callback;
            void* userData;
        };
        CallbackEntry* m_callbacks = nullptr;
        uint32 m_callbackCount = 0;
        uint32 m_callbackCapacity = 0;

        bool m_deviceLost = false;
        bool m_autoPolling = false;

        // スレッドセーフティ実装メモ:
        // AddCallback/RemoveCallback: lock_guard下でリスト操作
        // Poll(): ロック下でコールバックリストをローカルコピー → ロック解放 →
        //         コピーからコールバック呼び出し（各コールバックをtry/catchで保護）
        //         （コールバック内でのRegister/Removeによるデッドロック防止）
        //         （コールバック例外は捕捉・ログ出力して次のコールバックを継続）
    };

    //=========================================================================
    // RHIDeviceRecoveryOptions (17-02)
    //=========================================================================

    /// デバイス回復オプション
    struct RHI_API RHIDeviceRecoveryOptions
    {
        /// 自動再作成を試みる
        bool autoRecreate = true;

        /// 同じアダプターを使用（可能な場合）
        bool preferSameAdapter = true;

        /// リソースを再作成
        bool recreateResources = true;

        /// 回復タイムアウト（ミリ秒）
        uint32 timeoutMs = 5000;

        /// 最大リトライ回数
        uint32 maxRetries = 3;
    };

    //=========================================================================
    // RHIDeviceRecoveryManager (17-02)
    //=========================================================================

    /// デバイス回復マネージャー
    class RHI_API RHIDeviceRecoveryManager
    {
    public:
        RHIDeviceRecoveryManager() = default;

        /// 初期化
        void Initialize(IDynamicRHI* rhi, const RHIDeviceRecoveryOptions& options = {});

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // 回復
        //=====================================================================

        /// デバイス回復を試みる
        /// @return 回復に成功したか
        bool AttemptRecovery();

        /// 回復中か
        bool IsRecovering() const { return m_recovering; }

        /// 回復試行回数
        uint32 GetRecoveryAttempts() const { return m_recoveryAttempts; }

        //=====================================================================
        // リソース再作成
        //=====================================================================

        /// リソース再作成コールバック
        using ResourceRecreateCallback = void (*)(IRHIDevice* newDevice, void* userData);

        /// コールバック登録
        void AddResourceRecreateCallback(ResourceRecreateCallback callback, void* userData);

        /// コールバック削除
        void RemoveResourceRecreateCallback(ResourceRecreateCallback callback);

        //=====================================================================
        // 情報
        //=====================================================================

        /// 新しいデバイス取得（回復後）
        IRHIDevice* GetRecoveredDevice() const { return m_recoveredDevice; }

    private:
        IDynamicRHI* m_rhi = nullptr;
        RHIDeviceRecoveryOptions m_options;
        IRHIDevice* m_recoveredDevice = nullptr;

        bool m_recovering = false;
        uint32 m_recoveryAttempts = 0;

        struct RecreateCallbackEntry
        {
            ResourceRecreateCallback callback;
            void* userData;
        };
        RecreateCallbackEntry* m_recreateCallbacks = nullptr;
        uint32 m_recreateCallbackCount = 0;
    };

}} // namespace NS::RHI
