# 17-02: デバイスロスト

## 目的

GPUデバイスロストの検出と回復機能を定義する。

## 参照ドキュメント

- 09-03-gpu-event.md (RHIGPUCrashInfo)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIDeviceLost.h`

## TODO

### 1. デバイスロスト理由

```cpp
#pragma once

namespace NS::RHI
{
    /// デバイスロスト理由
    enum class ERHIDeviceLostReason : uint8
    {
        /// 不。
        Unknown,

        /// GPUハング（タイムアウト）
        Hung,

        /// GPUリセット
        Reset,

        /// ドライバのアップグレード
        DriverUpgrade,

        /// ドライバ内部のエラー
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
}
```

- [ ] ERHIDeviceLostReason 列挙型

### 2. デバイスロスト情報

```cpp
namespace NS::RHI
{
    /// デバイスロスト情報
    struct RHI_API RHIDeviceLostInfo
    {
        /// ロスト理由
        ERHIDeviceLostReason reason = ERHIDeviceLostReason::Unknown;

        /// ネイティブエラーコード（RESULT等）
        int32 nativeErrorCode = 0;

        /// 詳細メッセージ
        char message[512] = {};

        /// 最後に実行されたGPUコマンド（可能な場合）
        char lastGPUCommand[256] = {};

        /// ブレッドクラム情報（利用可能な場合）
        /// @note D3D12のDRED (Device Removed Extended Data) から取得。
        ///       09-03-gpu-event.md の RHIGPUCrashInfo::breadcrumbs と連携し、
        ///       GPU実行中の最後のブレッドクラムマーカーを記録する。
        uint32 lastBreadcrumbId = 0;
        char lastBreadcrumbMessage[256] = {};

        /// フォールトアドレス（ページフォールト時）。
        uint64 faultAddress = 0;

        /// タイムスタンプ
        uint64 timestamp = 0;
    };
}
```

- [ ] RHIDeviceLostInfo 構造体

### 3. デバイスロスト検出

```cpp
namespace NS::RHI
{
    /// デバイスロスト検出（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// デバイスがロストしていか
        virtual bool IsDeviceLost() const = 0;

        /// デバイスロスト情報取得
        virtual bool GetDeviceLostInfo(RHIDeviceLostInfo& outInfo) const = 0;

        /// デバイスロストをチェックしてログ出力
        bool CheckAndLogDeviceLost() const {
            if (IsDeviceLost()) {
                RHIDeviceLostInfo info;
                if (GetDeviceLostInfo(info)) {
                    NS_LOG_ERROR("[RHI] Device Lost: %s - %s",
                        GetDeviceLostReasonName(info.reason),
                        info.message);
                }
                return true;
            }
            return false;
        }
    };
}
```

- [ ] IsDeviceLost
- [ ] GetDeviceLostInfo

### 4. デバイスロストコールバック

```cpp
namespace NS::RHI
{
    /// デバイスロストコールバック
    using RHIDeviceLostCallback = void(*)(
        IRHIDevice* device,
        const RHIDeviceLostInfo& info,
        void* userData);

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
        void AddCallback(RHIDeviceLostCallback callback, void* userData = nullptr);

        /// コールバック削除
        void RemoveCallback(RHIDeviceLostCallback callback);

        //=====================================================================
        // ポーリング
        //=====================================================================

        /// デバイスロストをポーリング
        /// ロストしていればコールバックを呼び出す
        /// @return デバイスがロストしたか
        bool Poll();

        /// 自動ポーリング有効化（別スレッド）。
        void EnableAutoPolling(uint32 intervalMs = 100);

        /// 自動ポーリング無効化
        void DisableAutoPolling();

    private:
        IRHIDevice* m_device = nullptr;

        struct CallbackEntry {
            RHIDeviceLostCallback callback;
            void* userData;
        };
        CallbackEntry* m_callbacks = nullptr;
        uint32 m_callbackCount = 0;
        uint32 m_callbackCapacity = 0;

        /// コールバックリストの排他制御
        /// AddCallback/RemoveCallbackとPoll()間のデータ競合を防止
        std::mutex m_callbackMutex;

        bool m_deviceLost = false;
        bool m_autoPolling = false;

        // --- スレッドセーフティ実装メモ ---
        // AddCallback(): lock_guard(m_callbackMutex) 下でリスト操作
        // RemoveCallback(): lock_guard(m_callbackMutex) 下でリスト操作
        // Poll(): ロック下でコールバックリストをローカルコピー → ロック解放 →
        //         コピーからコールバック呼び出し（各コールバックをtry/catchで保護）
        //         （コールバック内でのRegister/Removeによるデッドロック防止）
        //         （コールバック例外は捕捉・ログ出力して次のコールバックを継続）
    };
}
```

- [ ] RHIDeviceLostCallback
- [ ] RHIDeviceLostHandler クラス

### 5. デバイス回復

```cpp
namespace NS::RHI
{
    /// デバイス回復オプション
    struct RHI_API RHIDeviceRecoveryOptions
    {
        /// 自動作成を試みる
        bool autoRecreate = true;

        /// 同じアダプターを使用（可能な場合）
        bool preferSameAdapter = true;

        /// リソースをの作成
        bool recreateResources = true;

        /// 回復タイムアウト（ミリ秒）
        uint32 timeoutMs = 5000;

        /// 最大リトライ回数
        uint32 maxRetries = 3;
    };

    /// デバイス回復マネージャー
    class RHI_API RHIDeviceRecoveryManager
    {
    public:
        RHIDeviceRecoveryManager() = default;

        /// 初期化
        void Initialize(
            IDynamicRHI* rhi,
            const RHIDeviceRecoveryOptions& options = {});

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

        /// リソース再作成コールバック登録
        using ResourceRecreateCallback = void(*)(IRHIDevice* newDevice, void* userData);
        void AddResourceRecreateCallback(ResourceRecreateCallback callback, void* userData);

        /// リソース再作成コールバック削除
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

        struct RecreateCallbackEntry {
            ResourceRecreateCallback callback;
            void* userData;
        };
        RecreateCallbackEntry* m_recreateCallbacks = nullptr;
        uint32 m_recreateCallbackCount = 0;
    };
}
```

- [ ] RHIDeviceRecoveryOptions 構造体
- [ ] RHIDeviceRecoveryManager クラス

## 検証方法

- [ ] デバイスロスト検出
- [ ] コールバック呼び出し
- [ ] デバイス回復
- [ ] リソース再作成
