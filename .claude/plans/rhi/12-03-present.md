# 12-03: Present操作

## 目的

スワップチェーンのPresent（画面表示（操作を定義する。

## 参照ドキュメント

- 12-02-swapchain-interface.md (IRHISwapChain)
- 01-25-queue-interface.md (IRHIQueue)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHISwapChain.h` (部分

## TODO

### 1. Presentパラメータ

```cpp
namespace NS::RHI
{
    /// Presentフラグ
    enum class ERHIPresentFlags : uint32
    {
        None = 0,

        /// テストのみ（実際には表示しない
        Test = 1 << 0,

        /// 次の垂直ブランクを待機しない
        DoNotWait = 1 << 1,

        /// フレームを破棄（次のPresentまで表示しない
        RestartFrame = 1 << 2,

        /// ティアリング許可（VSync無効時）
        AllowTearing = 1 << 3,

        /// ステレオ左目のみ
        StereoPreferRight = 1 << 4,

        /// ダーティリージョン使用
        UseDirtyRects = 1 << 5,

        /// スクロール使用
        UseScrollRect = 1 << 6,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIPresentFlags)

    /// ダーティリージョン
    struct RHI_API RHIDirtyRect
    {
        int32 left = 0;
        int32 top = 0;
        int32 right = 0;
        int32 bottom = 0;
    };

    /// スクロールリージョン
    struct RHI_API RHIScrollRect
    {
        RHIDirtyRect source;
        int32 offsetX = 0;
        int32 offsetY = 0;
    };

    /// Presentパラメータ
    struct RHI_API RHIPresentParams
    {
        /// フラグ
        ERHIPresentFlags flags = ERHIPresentFlags::None;

        /// 同期間隔
        /// 0: 即時（ティアリング可能性あり）。
        /// 1: 1 VSync（通常）。
        /// 2-4: 複数VSync待ち
        uint32 syncInterval = 1;

        /// ダーティリージョン配列
        const RHIDirtyRect* dirtyRects = nullptr;

        /// ダーティリージョン数
        uint32 dirtyRectCount = 0;

        /// スクロールリージョン
        const RHIScrollRect* scrollRect = nullptr;
    };
}
```

- [ ] ERHIPresentFlags 列挙型
- [ ] RHIDirtyRect 構造体
- [ ] RHIPresentParams 構造体

### 2. Present操作

```cpp
namespace NS::RHI
{
    /// Present結果
    enum class ERHIPresentResult : uint8
    {
        /// 成功。次フレームの描画を続行可能。
        Success,

        /// オクルード（ウィンドウが最小化等）。描画をスキップし、短いスリープ後にリトライ。
        Occluded,

        /// デバイスリセット。全リソースを再作成する必要がある。
        DeviceReset,

        /// デバイスロスト。デバイスを再作成し、全リソースを再構築する必要がある。
        DeviceLost,

        /// フレームがスキップされた。次フレームの描画を続行可能。
        FrameSkipped,

        /// タイムアウト。フレームレイテンシ待機でタイムアウト。リトライ可能。
        Timeout,

        /// エラー。ログを確認し、スワップチェーンの再作成を検討。
        Error,
    };

    /// Present操作（RHISwapChainに追加）。
    class IRHISwapChain
    {
    public:
        /// Present
        /// @param params パラメータ
        /// @return 結果
        virtual ERHIPresentResult Present(const RHIPresentParams& params = {}) = 0;

        /// 簡易Present
        ERHIPresentResult Present(uint32 syncInterval) {
            RHIPresentParams params;
            params.syncInterval = syncInterval;
            return Present(params);
        }

        /// VSync有効Present
        ERHIPresentResult PresentVSync() {
            return Present(1);
        }

        /// 即時Present（ティアリング）。
        ERHIPresentResult PresentImmediate() {
            RHI_ASSERT(HasFlag(GetFlags(), ERHISwapChainFlags::AllowTearing),
                "PresentImmediate requires AllowTearing flag on swapchain creation");
            RHIPresentParams params;
            params.syncInterval = 0;
            params.flags = ERHIPresentFlags::AllowTearing;
            return Present(params);
        }
    };
}
```

- [ ] ERHIPresentResult 列挙型
- [ ] Present操作

### 3. Present統計

```cpp
namespace NS::RHI
{
    /// フレーム統計
    struct RHI_API RHIFrameStatistics
    {
        /// 総のレゼント数
        uint64 presentCount = 0;

        /// 総リフレットュ数
        uint64 presentRefreshCount = 0;

        /// 同期リフレットュ数
        uint64 syncRefreshCount = 0;

        /// 同期QPCタイム
        uint64 syncQPCTime = 0;

        /// GPU時間（ナノ秒）
        uint64 syncGPUTime = 0;

        /// フレーム統計計算時のフレーム番号
        uint64 frameNumber = 0;
    };

    /// Present統計（RHISwapChainに追加）。
    class IRHISwapChain
    {
    public:
        /// フレーム統計取得
        virtual bool GetFrameStatistics(RHIFrameStatistics& outStats) const = 0;

        /// 最後のPresentのID取得
        virtual uint64 GetLastPresentId() const = 0;

        /// 指定PresentのGPU完了時刻得）。
        virtual bool WaitForPresentCompletion(uint64 presentId, uint64 timeoutMs = UINT64_MAX) = 0;
    };
}
```

- [ ] RHIFrameStatistics 構造体
- [ ] GetFrameStatistics

### 4. 構成変更Present

```cpp
namespace NS::RHI
{
    /// 構成変更Present
    /// バッファ設定変更を伴うPresent
    class IRHISwapChain
    {
    public:
        /// バッファ設定変更を伴うPresent
        /// @param width 新しい幅
        /// @param height 新しい高さ
        /// @param format 新しいフォーマット
        /// @param flags 新しいフラグ
        virtual ERHIPresentResult PresentAndResize(
            uint32 width,
            uint32 height,
            ERHIPixelFormat format = ERHIPixelFormat::Unknown,
            ERHISwapChainFlags flags = ERHISwapChainFlags::None) = 0;
    };
}
```

- [ ] PresentAndResize

### 5. マルチスワップチェーンPresent

```cpp
namespace NS::RHI
{
    /// 複数スワップチェーン同期Present
    /// 複数ウィンドウを同期して更新
    class RHI_API RHIMultiSwapChainPresenter
    {
    public:
        RHIMultiSwapChainPresenter() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // スワップチェーン管理
        //=====================================================================

        /// スワップチェーン追加
        void AddSwapChain(IRHISwapChain* swapChain);

        /// スワップチェーン削除
        void RemoveSwapChain(IRHISwapChain* swapChain);

        /// スワップチェーンクリア
        void ClearSwapChains();

        //=====================================================================
        // 同期Present
        //=====================================================================

        /// すべてのスワップチェーンをPresent
        /// @param syncInterval 同期間隔
        void PresentAll(uint32 syncInterval = 1);

        /// 指定スワップチェーンのみPresent
        void Present(IRHISwapChain* swapChain, uint32 syncInterval = 1);

        //=====================================================================
        // 情報
        //=====================================================================

        /// スワップチェーン数
        uint32 GetSwapChainCount() const { return m_swapChainCount; }

    private:
        IRHIDevice* m_device = nullptr;
        IRHISwapChain** m_swapChains = nullptr;
        uint32 m_swapChainCount = 0;
        uint32 m_swapChainCapacity = 0;
    };
}
```

- [ ] RHIMultiSwapChainPresenter クラス

## 検証方法

- [ ] Present動作
- [ ] VSync同期
- [ ] ダーティリージョン更新
- [ ] フレーム統計取得
