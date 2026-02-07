# 00-02: RHIエラー処理パターン

## 概要

RHI全体で一貫したエラー処理パターンを定義。
回復戦略、診断情報を統合。

## エラー処理方針

RHIはResult型を持たない。エラーは以下のパターンで処理する：

1. **リソース作成**: `TRefCountPtr<T>` を返す。失敗時は `nullptr`。
   - 呼び出し元で `nullptr` チェック
   - 失敗原因は内部ログに出力

2. **操作（Present等）**: `void` または `bool`。
   - 致命的エラーは `RHIDeviceLostHandler` コールバックで通知
   - 回復不能エラーはログ + assert

3. **バリデーション**: `RHI_CHECK` / `RHI_ENSURE` マクロ（デバッグビルドのみ）
   - 17-03-validation.md で定義

4. **致命的GPU障害**: 専用コールバックシステム
   - DeviceLost → `RHIDeviceLostHandler`
   - メモリ不足 → `RHIMemoryPressureHandler`

## 回復戦略

### デバイスロスト回復フロー

```
┌─────────────────────────────────────────────────────────────┐
│                  Device Lost 検出                          │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│             1. 進行中のコマンドを中止                       │
│ - ExecuteCommandLists の戻り値チェック                     │
│ - フェンス待機をキャンセル                                 │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│             2. 全リソースを無効化                          │
│ - IRHIResource::Invalidate() 呼び出し                     │
│ - 参照カウントは維持（ハンドル保持）                      │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│             3. デバイス再作成                               │
│ - 新しいアダプターを選択                                  │
│ - CreateDevice() 再実行                                   │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│             4. リソース再作成                               │
│ - 優先度順に再作成（PSO → バッファ → テクスチャ）         │
│ - 非同期で段階的に実行                                    │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│             5. レンダリング再開                             │
│ - スワップチェーン再作成                                   │
│ - フレーム描画を再開                                      │
└─────────────────────────────────────────────────────────────┘
```

### 実装コード

```cpp
namespace NS::RHI
{

/// デバイスロストハンドラ
class RHIDeviceLostHandler
{
public:
    /// コールバック型
    using Callback = std::function<void(ERHIDeviceLostReason reason)>;

    /// コールバック登録
    void RegisterCallback(Callback callback)
    {
        m_callbacks.push_back(std::move(callback));
    }

    /// デバイスロストを通知
    void OnDeviceLost(ERHIDeviceLostReason reason)
    {
        m_isDeviceLost = true;
        m_lostReason = reason;

        for (const auto& callback : m_callbacks)
        {
            try
            {
                callback(reason);
            }
            catch (...)
            {
                NS_LOG_ERROR("[RHI] Exception in DeviceLost callback - continuing");
            }
        }
    }

    /// デバイスロスト状態か
    bool IsDeviceLost() const { return m_isDeviceLost; }

    /// ロスト理由取得
    ERHIDeviceLostReason GetLostReason() const { return m_lostReason; }

    /// 復旧完了通知
    void OnDeviceRecovered()
    {
        m_isDeviceLost = false;
        m_lostReason = ERHIDeviceLostReason::Unknown;
    }

private:
    std::vector<Callback> m_callbacks;
    std::atomic<bool> m_isDeviceLost{false};
    ERHIDeviceLostReason m_lostReason = ERHIDeviceLostReason::Unknown;
};

/// リソース再作成マネージャー
class RHIResourceRecoveryManager
{
public:
    /// 再作成が必要なリソースを登録
    template<typename T>
    void RegisterRecreatable(
        TRefCountPtr<T>& resource,
        std::function<T*()> createFunc)
    {
        m_recreatables.push_back({
            // type-erased reset: 古いリソースを解放し新しいリソースを設定
            [&resource, createFunc]() -> bool {
                T* newResource = createFunc();
                if (!newResource) return false;
                resource.Reset(newResource);  // TRefCountPtr::Reset で安全に差し替え
                newResource->Release();        // Reset内でAddRefされるため
                return true;
            }
        });
    }

    /// 全リソースを再作成
    bool RecreateAll()
    {
        for (auto& entry : m_recreatables)
        {
            if (!entry.recreateFunc())
            {
                NS_LOG_ERROR("[RHI] Failed to recreate resource");
                return false;
            }
        }
        return true;
    }

private:
    struct RecreatableEntry
    {
        std::function<bool()> recreateFunc;
    };
    std::vector<RecreatableEntry> m_recreatables;
};

} // namespace NS::RHI
```

## メモリ不足対応

```cpp
namespace NS::RHI
{

/// メモリプレッシャーレベル
enum class ERHIMemoryPressure : uint8
{
    None,       ///< 正常
    Low,        ///< 軽度（警告）
    Medium,     ///< 中度（不要リソース解放推奨）
    High,       ///< 高度（強制解放開始）
    Critical,   ///< 致命的（割り当て失敗）
};

/// メモリプレッシャーハンドラ
class RHIMemoryPressureHandler
{
public:
    using Callback = std::function<void(ERHIMemoryPressure level)>;

    void RegisterCallback(Callback callback);
    void NotifyPressureChange(ERHIMemoryPressure newLevel);

    /// 現在のプレッシャーレベル
    ERHIMemoryPressure GetCurrentLevel() const { return m_currentLevel; }

    /// 推奨アクション
    static const char* GetRecommendedAction(ERHIMemoryPressure level)
    {
        switch (level)
        {
            case ERHIMemoryPressure::Low:
                return "Consider releasing unused resources";
            case ERHIMemoryPressure::Medium:
                return "Release streaming textures and LOD data";
            case ERHIMemoryPressure::High:
                return "Force eviction of non-essential resources";
            case ERHIMemoryPressure::Critical:
                return "Allocation may fail - reduce quality settings";
            default:
                return "No action needed";
        }
    }

private:
    ERHIMemoryPressure m_currentLevel = ERHIMemoryPressure::None;
    std::vector<Callback> m_callbacks;
};

} // namespace NS::RHI
```

## 検証

- [ ] 回復フローの動作
- [ ] ログ出力の可読性
